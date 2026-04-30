#pragma once

#include <atomic>
#include <chrono>
#include <format>
#include <functional>
#include <list>
#include <mutex>
#include <semaphore>
#include <stop_token>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "best_functions.h"
#include "common.h"
#include "comparison.h"
#include "func_node.h"
#include "status.h"
#include "target.h"

namespace fw
{

/// @addtogroup Search
/// @{

template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class SearchWorker
{
   public:
    /// Type alias for function nodes used in this search
    using FN_t = FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;
    using BestPool_t = BestPool<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;
    using TimePoint_t = std::chrono::time_point<std::chrono::steady_clock>;

    explicit SearchWorker(Settings settings, AtomFuncs<FuncValue_t>* atoms, Target<FuncValue_t>* target,
                          uint worker_num, std::binary_semaphore& sem)
        : m_settings(std::move(settings)),
          m_atoms(atoms),
          m_target(target),
          m_worker_num(worker_num),
          m_fn(atoms),
          m_sem(sem)
    {
    }

    uint Num() const { return m_worker_num; }

    bool operator==(const SearchWorker<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other) const
    {
        if (m_settings != other.m_settings) {
            return false;
        }
        if (m_atoms != other.m_atoms) {
            return false;
        }
        if (m_target != other.m_target) {
            return false;
        }
        if (m_fn != other.m_fn) {
            return false;
        }
        if (m_count != other.m_count) {
            return false;
        }
        if (m_best_pool != other.m_best_pool) {
            return false;
        }
        if (m_done != other.m_done) {
            return false;
        }
        return true;
    }

    bool Iterate() { return m_fn.Iterate(m_settings.max_depth); }

    void Run() { m_thread = std::jthread(std::bind_front(&SearchWorker::Search, this)); }

    void Stop()
    {
        m_thread.request_stop();
        m_thread.join();
    }

    [[nodiscard]] bool Done() const { return m_done; }

    void NewTask(SerialNumber_t snum_from, SerialNumber_t snum_to)
    {
        assert(snum_from < snum_to);
        m_task_count += 1;
        m_snum_from = snum_from;
        m_snum_to = snum_to;
        m_fn.FromSerialNumber(m_snum_from);
        m_done = false;
    }

    status::WorkerStatus GetStatus()
    {
        status::WorkerStatus status;
        const std::lock_guard<std::mutex> lock(m_mtx);
        status.func_serial_number = m_fn.SerialNumber();
        status.current_function = m_fn.Repr();
        status.func_serial_number_from = m_snum_from;
        status.func_serial_number_to = m_snum_to;
        status.done = m_done;
        status.done_percent = ((status.func_serial_number - status.func_serial_number_from) * 100.0F) /
                              (status.func_serial_number_to - status.func_serial_number_from);
        status.task_count = m_task_count;

        status.best_functions.reserve(m_best_pool.Functions().size());
        for (auto& best : m_best_pool.Functions()) {
            status::BestFunc best_func;
            best_func.function = best.Repr();
            best_func.suit = CalcDist(best, m_target);
            best_func.match_positions = m_target->MatchPositions(best.Calculate()).Str();
            status.best_functions.push_back(best_func);
        }
        return status;
    }

    BestPool_t GetBest() const
    {
        const std::lock_guard<std::mutex> lock(m_mtx);
        return m_best_pool;
    }

    SerialNumber_t GetSNumLast() const { return m_snum_last; }

   private:
    Settings m_settings;                        ///< ⚙️ Search configuration parameters
    AtomFuncs<FuncValue_t>* m_atoms = nullptr;  ///< 🧩 Reference to atomic function library
    Target<FuncValue_t>* m_target = nullptr;    ///< 🎯 Reference to target specification
    uint m_worker_num = 0;

    FN_t m_fn;                ///< 🌳 Current function being evaluated
    std::size_t m_count = 0;  ///< 🔢 Number of iterations performed
    BestPool_t m_best_pool;
    mutable std::mutex m_mtx;        ///< 🔐 Mutex for thread-safe state access
    std::jthread m_thread;           ///< 🧵 Background search thread
    std::atomic_bool m_done = true;  ///< ✅ Completion flag (atomic for thread safety)
    uint64_t m_task_count = 0;
    SerialNumber_t m_snum_from = 0;
    SerialNumber_t m_snum_to = 0;
    SerialNumber_t m_snum_last = 0;

    std::binary_semaphore& m_sem;

    /**
     * @brief Main search loop (runs in background thread)
     * @param stoken Stop token for cooperative cancellation
     * 
     * Continuously iterates through function space until:
     * - Stop is requested via stop_token
     * - Search completes (no more functions)
     * - Done flag is set
     * 
     * Progress is automatically saved if save_file is configured.
     */
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    void Search(std::stop_token stoken)
    {
        std::println("    Search started (worker #{:3})", m_worker_num);

        while (not stoken.stop_requested()) {
            if (not m_done) {
                bool iterate_ok = true;
                while (iterate_ok and (not m_done) and (not stoken.stop_requested())) {
                    iterate_ok = SearchIterate();
                    const auto snum = m_fn.SerialNumber();
                    m_snum_last = snum;
                    if ((not iterate_ok) or (snum > m_snum_to)) {
                        m_done = true;
                        m_sem.release();
                    }
                }
            }
            else {
                constexpr auto PauseCheck = std::chrono::milliseconds(1);
                std::this_thread::sleep_for(PauseCheck);
            }
        }
    }

    /**
     * @brief Perform single search iteration (thread-safe)
     * @return true if iteration successful, false if search complete
     * 
     * This is the core search operation:
     * 1. Advance to next function
     * 2. Evaluate against target
     * 3. Update best list if warranted
     * 4. Increment iteration counter
     * 
     * Protected by mutex for thread safety when called from Search().
     */
    bool SearchIterate()
    {
        const std::lock_guard<std::mutex> lock(m_mtx);
        if (not m_fn.Iterate(m_settings.max_depth, 0, std::nullopt)) {
            return false;
        }
        m_best_pool.CheckBest(m_fn, m_target, m_settings.max_best);
        ++m_count;
        return true;
    }
};

/// @} // end of Search group

}  // namespace fw