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
#include "search_worker.h"
#include "status.h"
#include "target.h"

namespace fw
{

/// @addtogroup Search
/// @{

template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class SearchManager
{
   public:
    /// Type alias for function nodes used in this search
    using FN_t = FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;
    using BestPool_t = BestPool<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;
    using TimePoint_t = std::chrono::time_point<std::chrono::steady_clock>;
    using SearchWorker_t = SearchWorker<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;

    explicit SearchManager(Settings settings, AtomFuncs<FuncValue_t>* atoms, Target<FuncValue_t>* target)
        : m_settings(std::move(settings)), m_atoms(atoms), m_target(target)
    {
        FN_t func{m_atoms};
        m_max_sn = func.MaxSerialNumber(m_settings.max_depth);
    }

    bool operator==(const SearchManager<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other) const
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

    void Run() { m_thread = std::jthread(std::bind_front(&SearchManager::Search, this)); }

    void StopGraceful() { m_graceful_stop = true; }

    void Stop()
    {
        m_thread.request_stop();
        m_thread.join();
    }

    [[nodiscard]] bool Done() const { return m_done; }

    /**
     * @brief Serialize search state to JSON
     * @return JSON object containing complete search state
     * 
     * Includes configuration, progress counters, current position,
     * and all best functions found so far.
     * 
     * @see FromJSON()
     */
    [[nodiscard]] json ToJSON() const
    {
        json j;
        j["settings"]["max_best"] = m_settings.max_best;
        j["settings"]["max_depth"] = m_settings.max_depth;
        j["count"] = m_count;
        j["done"] = m_done.load();
        j["suit_threshold"]["distance"] = m_best_pool.SuitThreshold().distance();
        j["suit_threshold"]["max_level"] = m_best_pool.SuitThreshold().max_level();
        j["suit_threshold"]["functions_count"] = m_best_pool.SuitThreshold().functions_count();
        j["suit_threshold"]["functions_unique"] = m_best_pool.SuitThreshold().functions_unique();
        j["current_serial_number"] = SerialNumberToString(m_snum);
        j["best"] = json::array();
        for (const auto& func : m_best_pool.FunctionsKit()) {
            j["best"].push_back(func.func.ToJSON());
        }
        return j;
    }

    /**
     * @brief Deserialize search state from JSON
     * @param json_str JSON string containing saved search state
     * @return true if deserialization successful, false on error
     * 
     * Restores search to exactly where it was when saved.
     * Can be used to resume interrupted searches.
     * 
     * @see ToJSON()
     */
    bool FromJSON(std::string_view json_str)
    {
        auto j = json::parse(json_str, nullptr, false);
        if (j.is_discarded()) {
            return false;
        }

        if (not j.is_object()) {
            return false;
        }

        const auto j_settings = j.find("settings");
        if (j_settings == j.end()) {
            return false;
        }
        if (not j_settings->is_object()) {
            return false;
        }

        const auto j_settings_max_best = j_settings->find("max_best");
        if (j_settings_max_best == j_settings->end()) {
            return false;
        }
        if (not j_settings_max_best->is_number()) {
            return false;
        }
        m_settings.max_best = j_settings_max_best->get<std::size_t>();

        const auto j_settings_max_depth = j_settings->find("max_depth");
        if (j_settings_max_depth == j_settings->end()) {
            return false;
        }
        if (not j_settings_max_depth->is_number()) {
            return false;
        }
        m_settings.max_depth = j_settings_max_depth->get<std::size_t>();

        const auto j_count = j.find("count");
        if (j_count == j.end()) {
            return false;
        }
        if (not j_count->is_number()) {
            return false;
        }
        m_count = j_count->get<std::size_t>();

        const auto j_done = j.find("done");
        if (j_done == j.end()) {
            return false;
        }
        if (not j_done->is_boolean()) {
            return false;
        }
        m_done = j_done->get<bool>();

        const auto j_suit_threshold = j.find("suit_threshold");
        if (j_suit_threshold == j.end()) {
            return false;
        }
        if (not j_suit_threshold->is_object()) {
            return false;
        }

        const auto j_suit_threshold_distance = j_suit_threshold->find("distance");
        if (j_suit_threshold_distance == j_suit_threshold->end()) {
            return false;
        }
        if (not j_suit_threshold_distance->is_number()) {
            return false;
        }
        const auto j_suit_threshold_max_level = j_suit_threshold->find("max_level");
        if (j_suit_threshold_max_level == j_suit_threshold->end()) {
            return false;
        }
        if (not j_suit_threshold_max_level->is_number()) {
            return false;
        }
        const auto j_suit_threshold_functions_count = j_suit_threshold->find("functions_count");
        if (j_suit_threshold_functions_count == j_suit_threshold->end()) {
            return false;
        }
        if (not j_suit_threshold_functions_count->is_number()) {
            return false;
        }
        const auto j_suit_threshold_functions_unique = j_suit_threshold->find("functions_unique");
        if (j_suit_threshold_functions_unique == j_suit_threshold->end()) {
            return false;
        }
        if (not j_suit_threshold_functions_unique->is_number()) {
            return false;
        }

        m_best_pool.SetSuitThreshold(SuitabilityMetrics(j_suit_threshold_distance->get<std::size_t>(),
                                                        j_suit_threshold_max_level->get<std::size_t>(),
                                                        j_suit_threshold_functions_count->get<std::size_t>(),
                                                        j_suit_threshold_functions_unique->get<std::size_t>()));

        const auto j_current_serial_number = j.find("current_serial_number");
        if (j_current_serial_number == j.end()) {
            return false;
        }
        if (not j_current_serial_number->is_string()) {
            return false;
        }
        const auto snum = StringToSerialNumber(j_current_serial_number->get<std::string>());
        if (not snum.has_value()) {
            return false;
        }
        m_snum = *snum;

        m_best_pool.Functions().clear();
        const auto j_best = j.find("best");
        if (j_best != j.end()) {
            if (not j_best->is_array()) {
                return false;
            }

            for (auto& j_best_it : *j_best) {
                m_best_pool.Functions().emplace_back(m_atoms);
                if (not m_best_pool.Functions().back().FromJSON(j_best_it)) {
                    return false;
                }
            }
        }

        return true;
    }

    std::string Status()
    {
        const std::lock_guard<std::mutex> lock(m_mtx);
        const float done_percent = (m_snum * 100.0F) / m_max_sn;

        const auto elapsed = std::chrono::steady_clock::now() - m_tm_start;
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        if (d == 0) {
            d = 1;
        }
        const std::size_t c_per_sec = m_count * 1000 / d;

        const auto sn_per_sec = m_snum * 1000 / d;
        const auto remaining_sn = m_max_sn - m_snum;
        const auto remaining = std::chrono::seconds(remaining_sn / sn_per_sec);

        const auto remaining_h = std::chrono::duration_cast<std::chrono::hours>(remaining);
        const auto remaining_m = std::chrono::duration_cast<std::chrono::minutes>(remaining % std::chrono::hours(1));
        const auto remaining_s = std::chrono::duration_cast<std::chrono::seconds>(remaining % std::chrono::minutes(1));

        const auto elapsed_h = std::chrono::duration_cast<std::chrono::hours>(elapsed);
        const auto elapsed_m = std::chrono::duration_cast<std::chrono::minutes>(elapsed % std::chrono::hours(1));
        const auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsed % std::chrono::minutes(1));

        auto status = std::format(
            "iteration {}; func sn {} from max {}; progress {}%; speed {} ips; elapsed: {}:{:02d}:{:02d}; remaining: "
            "{}:{:02d}:{:02d}\n",
            format_with_si_prefix(m_count), format_with_si_prefix(m_snum), format_with_si_prefix(m_max_sn),
            done_percent, format_with_si_prefix(c_per_sec), elapsed_h.count(), elapsed_m.count(), elapsed_s.count(),
            remaining_h.count(), remaining_m.count(), remaining_s.count());

        status += std::format("|  dist  | lvl | fnc | fnu | {:48}| coincidences\n", "function");
        for (auto& best_func : m_best_pool.Functions()) {
            const auto suit = CalcDist(best_func);
            status += std::format("| {:6} | {:3} | {:3} | {:3} | {:48}| {} \n", suit.distance(), suit.max_level(),
                                  suit.functions_count(), suit.functions_unique(), best_func.Repr(),
                                  m_target->MatchPositions(best_func.Calculate()).Str());
        }

        std::size_t worker_count = 0;
        status += std::format("workers current functions:\n");
        for (const auto& worker_status : m_status.workers_status) {
            status += std::format("| #{:2} | {} \n", worker_count, worker_status.current_function);
            ++worker_count;
        }
        return status;
    }

    status::Status GetStatus()
    {
        const std::lock_guard<std::mutex> lock(m_mtx);
        return m_status;
    }

    void CollectStatus()
    {
        const std::lock_guard<std::mutex> lock(m_mtx);
        m_status.func_serial_number = m_snum;
        m_status.max_func_serial_number = m_max_sn;
        m_status.done_percent = (m_status.func_serial_number * 100.0F) / m_status.max_func_serial_number;
        m_status.elapsed = std::chrono::steady_clock::now() - m_tm_start;
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(m_status.elapsed).count();
        if (d == 0) {
            d = 1;
        }
        m_status.iterations_count = m_count;
        m_status.iterations_per_sec = m_status.iterations_count * 1000 / d;
        m_status.sn_per_sec = m_status.func_serial_number * 1000 / d;
        const auto remaining_sn = m_status.max_func_serial_number - m_status.func_serial_number;
        m_status.remaining = std::chrono::seconds(m_status.sn_per_sec != 0 ? remaining_sn / m_status.sn_per_sec : 0);

        m_best_pool.Reset();
        m_status.workers_status.clear();
        for (auto& worker : m_workers) {
            auto worker_best = worker->GetBest();
            m_best_pool.CheckBest(worker_best, m_target, m_settings.max_best);
            const auto worker_status = worker->GetStatus();
            m_status.workers_status.push_back(worker_status);
        }

        m_status.best_functions.clear();
        m_status.best_functions.reserve(m_best_pool.Functions().size());
        for (auto& best : m_best_pool.Functions()) {
            status::BestFunc best_func;
            best_func.function = best.Repr();
            best_func.suit = CalcDist(best, m_target);
            best_func.match_positions = m_target->MatchPositions(best.Calculate()).Str();
            m_status.best_functions.push_back(best_func);
        }
    }

   private:
    using WorkersPool = std::vector<std::unique_ptr<SearchWorker_t>>;

    Settings m_settings;                        ///< ⚙️ Search configuration parameters
    AtomFuncs<FuncValue_t>* m_atoms = nullptr;  ///< 🧩 Reference to atomic function library
    Target<FuncValue_t>* m_target = nullptr;    ///< 🎯 Reference to target specification
    TimePoint_t m_tm_start;                     ///< ⏱️ Search start time
    std::size_t m_count = 0;                    ///< 🔢 Number of iterations performed
    SerialNumber_t m_snum = 0;                  ///< 🔢 Current serial number
    SerialNumber_t m_max_sn = 0;                ///< 🛑 Upper limit for serial numbers
    SerialNumber_t m_task_step = 0;             ///< 📏 Step size for serial numbers when dispatching tasks
    BestPool_t m_best_pool;                     ///< 🏆 Pool holding the best functions
    mutable std::mutex m_mtx;                   ///< 🔐 Mutex for thread-safe state access
    WorkersPool m_workers;                      ///< 👷‍♂️ Collection of worker objects (parallel search tasks)
    std::jthread m_thread;                      ///< 🧵 Background search thread
    std::atomic_bool m_graceful_stop = false;   ///< 🙏 Flag to request graceful stop
    std::atomic_bool m_done = false;            ///< ✅ Completion flag (atomic for thread safety)
    status::Status m_status;                    ///< 📊 Current status of the search
    std::binary_semaphore m_sem_workers{0};     ///< 🔔 Semaphore for worker synchronisation

    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    void Search(std::stop_token stoken)
    {
        const auto theads_count = (m_settings.threads == 0) ? std::thread::hardware_concurrency() : m_settings.threads;

        m_task_step = m_max_sn / (m_settings.tasks_per_worker_count * theads_count);
        std::println("    Search started (step={})", m_task_step);
        m_tm_start = std::chrono::steady_clock::now();

        for (std::size_t i = 0; i < theads_count; ++i) {
            m_workers.emplace_back(new SearchWorker_t(m_settings, m_atoms, m_target, i, m_sem_workers));
            m_workers.back()->Run();
        }

        while ((not stoken.stop_requested()) and (not m_done)) {
            bool all_workers_done = true;
            for (auto& worker : m_workers) {
                if (worker->Done()) {
                    const auto snum_last = worker->GetSNumLast();
                    if (m_snum < snum_last) {
                        m_snum = snum_last;
                    }
                    if ((m_snum <= m_max_sn) and (not m_graceful_stop)) {
                        const auto snum_from = m_snum;
                        auto snum_to = m_snum + m_task_step;
                        if (snum_to > m_max_sn) {
                            snum_to = m_max_sn + 1;
                        }
                        m_snum = snum_to;
                        worker->NewTask(snum_from, snum_to);
                    }
                }
                else {
                    all_workers_done = false;
                }
            }

            if (all_workers_done and ((m_snum > m_max_sn) or m_graceful_stop)) {
                m_done = true;
            }
            else {
                constexpr auto PauseCheck = std::chrono::milliseconds(10);
                m_sem_workers.try_acquire_for(PauseCheck);
                CollectStatus();
            }
        }

        for (auto& worker : m_workers) {
            worker->Stop();
        }
    }
};

/// @} // end of Search group

}  // namespace fw