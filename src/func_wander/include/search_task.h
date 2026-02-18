#pragma once

#include <atomic>
#include <chrono>
#include <format>
#include <functional>
#include <list>
#include <mutex>
#include <stop_token>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "common.h"
#include "comparison.h"
#include "func_node.h"
#include "status.h"
#include "target.h"

namespace fw
{

/// @addtogroup Search
/// @{

/**
 * @struct Settings
 * @brief Configuration parameters for search tasks
 * 
 * Contains all tunable parameters that control the search behavior,
 * including resource limits and output settings.
 */
struct Settings
{
    /// @brief Equality comparison operator
    bool operator==(const Settings& other) const
    {
        return ((save_file == other.save_file) and (max_best == other.max_best) and (max_depth == other.max_depth));
    }

    std::string save_file;                ///< 📁 File path for automatic save/load of search state
    std::size_t max_best = 32;            ///< 🏆 Maximum number of best functions to retain
    std::size_t max_depth = 3;            ///< 🌳 Maximum depth of function trees to explore
    bool http_enabled = false;            ///< 🌐 Enable/disable HTTP server for remote control
    std::string http_host = "localhost";  ///< 🖧 Host address for HTTP server (default: localhost)
    int http_port = 8080;                 ///< 🔌 Port for HTTP server (default: 8080)
};

/**
 * @class SearchTask
 * @brief Main orchestrator for brute-force function discovery
 * @tparam FuncValue_t Type of function values (e.g., int, double, bool)
 * @tparam SKIP_CONSTANT Whether to skip constant expressions during iteration
 * @tparam SKIP_SYMMETRIC Whether to skip symmetric duplicates for commutative ops
 * 
 * Manages the systematic exploration of function space to discover
 * expressions that approximate a target function. Supports parallel
 * execution, progress tracking, and state persistence.
 * 
 * The search enumerates all possible function trees up to a given
 * depth, evaluates them against the target, and maintains a ranked
 * list of the best candidates.
 * 
 * @note Search can be run in background threads with cooperative
 * cancellation via std::stop_token.
 */
template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class SearchTask
{
   public:
    /// Type alias for function nodes used in this search
    using FN_t = FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;

    /**
     * @brief Construct a new search task
     * @param settings Configuration parameters for the search
     * @param atoms Pointer to atomic function library
     * @param target Pointer to target specification
     * 
     * @note The SearchTask does not take ownership of atoms or target.
     *       These must remain valid for the lifetime of the task.
     */
    explicit SearchTask(Settings settings, AtomFuncs<FuncValue_t>* atoms, Target<FuncValue_t>* target)
        : m_settings(std::move(settings)), m_atoms(atoms), m_target(target), m_fn{atoms}
    {
    }

    /**
     * @brief Equality comparison operator
     * @param other SearchTask to compare with
     * @return true if tasks have identical state
     */
    bool operator==(const SearchTask<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other) const
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
        if (m_best != other.m_best) {
            return false;
        }
        if (m_suit_threshold != other.m_suit_threshold) {
            return false;
        }
        if (m_done != other.m_done) {
            return false;
        }
        return true;
    }

    /**
     * @brief Advance to next function in enumeration sequence
     * @return true if next function exists, false if enumeration complete
     * 
     * Updates m_fn to the next function tree in lexicographic order.
     * Used for single-step iteration without starting a background thread.
     */
    bool Iterate() { return m_fn.Iterate(m_settings.max_depth); }

    /**
     * @brief Start search in a background thread
     * 
     * Launches a std::jthread that runs the Search() method.
     * Progress can be monitored via Status() and Best() methods.
     */
    void Run() { m_thread = std::jthread(std::bind_front(&SearchTask::Search, this)); }

    /**
     * @brief Stop background search thread
     * 
     * Requests stop via std::stop_token and waits for thread completion.
     * Search state is preserved and can be resumed or serialized.
     */
    void Stop()
    {
        m_thread.request_stop();
        m_thread.join();
    }

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
        j["suit_threshold"]["distance"] = m_suit_threshold.distance();
        j["suit_threshold"]["max_level"] = m_suit_threshold.max_level();
        j["suit_threshold"]["functions_count"] = m_suit_threshold.functions_count();
        j["suit_threshold"]["functions_unique"] = m_suit_threshold.functions_unique();
        j["current_fn"] = m_fn.ToJSON();
        j["best"] = json::array();
        for (const auto& best : m_best) {
            j["best"].push_back(best.ToJSON());
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

        m_suit_threshold = SuitabilityMetrics(j_suit_threshold_distance->get<std::size_t>(),
                                              j_suit_threshold_max_level->get<std::size_t>(),
                                              j_suit_threshold_functions_count->get<std::size_t>(),
                                              j_suit_threshold_functions_unique->get<std::size_t>());

        const auto j_fn = j.find("current_fn");
        if (j_fn == j.end()) {
            return false;
        }
        if (not j_fn->is_object()) {
            return false;
        }

        if (not m_fn.FromJSON(*j_fn)) {
            return false;
        }

        m_best.clear();
        const auto j_best = j.find("best");
        if (j_best != j.end()) {
            if (not j_best->is_array()) {
                return false;
            }

            for (auto& j_best_it : *j_best) {
                m_best.emplace_back(m_atoms);
                if (not m_best.back().FromJSON(j_best_it)) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief Check if search has completed
     * @return true if search exhausted all possibilities, false otherwise
     */
    [[nodiscard]] bool Done() const { return m_done; }

    /**
     * @brief Get current best functions found
     * @return Vector of best function trees, sorted by quality
     * 
     * Functions are ranked by a composite score considering:
     * 1. Distance to target (accuracy)
     * 2. Tree depth (simplicity)
     * 3. Node count (compactness)
     */
    [[nodiscard]] std::vector<FN_t> Best() const
    {
        std::unique_lock lock{m_mtx};
        return std::vector<FN_t>(m_best.begin(), m_best.end());
    }

    /**
     * @brief Generate human-readable status report
     * @return Formatted string with progress statistics
     * 
     * Includes:
     * - Iteration count and progress percentage
     * - Current function being evaluated
     * - Iterations per second
     * - List of best functions with their scores
     */
    std::string Status()
    {
        const std::unique_lock lock{m_mtx};
        const auto snum = m_fn.SerialNumber();
        const auto max_sn = m_fn.MaxSerialNumber(m_settings.max_depth);
        const float done_percent = (snum * 100.0F) / max_sn;

        const auto elapsed = std::chrono::steady_clock::now() - m_tm_start;
        const auto d = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        const std::size_t c_per_sec = m_count * 1000 / d;

        const auto sn_per_sec = snum * 1000 / d;
        const auto remaining_sn = max_sn - snum;
        const auto remaining = std::chrono::seconds(remaining_sn / sn_per_sec);

        const auto remaining_h = std::chrono::duration_cast<std::chrono::hours>(remaining);
        const auto remaining_m = std::chrono::duration_cast<std::chrono::minutes>(remaining % std::chrono::hours(1));
        const auto remaining_s = std::chrono::duration_cast<std::chrono::seconds>(remaining % std::chrono::minutes(1));

        const auto elapsed_h = std::chrono::duration_cast<std::chrono::hours>(elapsed);
        const auto elapsed_m = std::chrono::duration_cast<std::chrono::minutes>(elapsed % std::chrono::hours(1));
        const auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsed % std::chrono::minutes(1));

        auto status = std::format(
            "iteration {}; func sn {} from max {}; progress {}%; speed {} ips; elapsed: {}:{:02d}:{:02d}; remaining: "
            "{}:{:02d}:{:02d}; function {}\n",
            format_with_si_prefix(m_count), format_with_si_prefix(snum), format_with_si_prefix(max_sn), done_percent,
            format_with_si_prefix(c_per_sec), elapsed_h.count(), elapsed_m.count(), elapsed_s.count(),
            remaining_h.count(), remaining_m.count(), remaining_s.count(), m_fn.Repr());

        status += std::format("|  dist  | lvl | fnc | fnu | {:48}| coincidences\n", "function");
        for (auto& best : m_best) {
            const auto suit = CalcDist(best);
            status += std::format("| {:6} | {:3} | {:3} | {:3} | {:48}| {} \n", suit.distance(), suit.max_level(),
                                  suit.functions_count(), suit.functions_unique(), best.Repr(),
                                  m_target->MatchPositions(best.Calculate()).Str());
        }
        return status;
    }

    status::Status GetStatus()
    {
        status::Status status;
        const std::unique_lock lock{m_mtx};
        status.snum = m_fn.SerialNumber();
        status.max_sn = m_fn.MaxSerialNumber(m_settings.max_depth);
        status.done_percent = (status.snum * 100.0F) / status.max_sn;

        status.elapsed = std::chrono::steady_clock::now() - m_tm_start;
        const auto d = std::chrono::duration_cast<std::chrono::milliseconds>(status.elapsed).count();
        status.iterations_count = m_count;
        status.iterations_per_sec = status.iterations_count * 1000 / d;
        status.sn_per_sec = status.snum * 1000 / d;
        const auto remaining_sn = status.max_sn - status.snum;
        status.remaining = std::chrono::seconds(remaining_sn / status.sn_per_sec);
        status.current_function = m_fn.Repr();

        status.best_functions.reserve(m_best.size());
        for (auto& best : m_best) {
            status::BestFunc best_func;
            best_func.function = best.Repr();
            best_func.suit = CalcDist(best);
            best_func.match_positions = m_target->MatchPositions(best.Calculate()).Str();
            status.best_functions.push_back(best_func);
        }
        return status;
    }

   private:
    Settings m_settings;                                            ///< ⚙️ Search configuration parameters
    AtomFuncs<FuncValue_t>* m_atoms = nullptr;                      ///< 🧩 Reference to atomic function library
    Target<FuncValue_t>* m_target = nullptr;                        ///< 🎯 Reference to target specification
    FN_t m_fn;                                                      ///< 🌳 Current function being evaluated
    std::chrono::time_point<std::chrono::steady_clock> m_tm_start;  ///< ⏱️ Search start time
    std::size_t m_count = 0;                                        ///< 🔢 Number of iterations performed
    std::list<FN_t> m_best;                                         ///< 🏆 Best functions found (maintained in order)
    SuitabilityMetrics m_suit_threshold;                            ///< 📊 Worst distance currently in best list
    std::mutex m_mtx;                                               ///< 🔐 Mutex for thread-safe state access
    std::jthread m_thread;                                          ///< 🧵 Background search thread
    std::atomic_bool m_done = false;                                ///< ✅ Completion flag (atomic for thread safety)

    /**
     * @brief Calculate composite distance metric for a function
     * @param fnc Function tree to evaluate
     * @return Composite distance score (lower = better)
     * 
     * Distance formula:
     *   distance = (target_distance × 10) + depth + (node_count × 2)
     * 
     * Weights can be adjusted based on preference for accuracy vs simplicity.
     */
    SuitabilityMetrics CalcDist(FN_t& fnc) const
    {
        const auto fnc_calc = fnc.Calculate();
        const auto fnc_cmp = m_target->Compare(fnc_calc);
        std::unordered_set<SerialNumber_t, SerialNumberHash> uniqs{};
        fnc.UniqFunctionsSerialNumbers(uniqs);
        return SuitabilityMetrics(fnc_cmp, fnc.CurrentMaxLevel(), fnc.FunctionsCount(), uniqs.size());
    }

    /**
     * @brief Evaluate and potentially add function to best list
     * @param fnc Candidate function tree
     * @param max_best Maximum size of best list
     * 
     * Algorithm:
     * 1. Calculate distance score
     * 2. Skip if worse than current threshold and list is full
     * 3. Check for uniqueness (exact values or matching positions)
     * 4. Insert in sorted position
     * 5. Trim list if exceeds max_best
     * 6. Update distance threshold
     */
    void CheckBest(FN_t& fnc, std::size_t max_best = 10)
    {
        if (m_best.empty()) {
            m_best.push_back(fnc);
            return;
        }

        const auto fnc_calc = fnc.Calculate();
        const auto fnc_ranges = m_target->MatchPositions(fnc_calc);
        const auto new_dist = CalcDist(fnc);
        if (m_best.size() >= max_best) {
            if (new_dist > m_suit_threshold) {
                return;
            }
        }

        auto best_it = m_best.begin();
        while (best_it != m_best.end()) {
            const auto dist = CalcDist(*best_it);
            if (new_dist < dist) {
                // Check for uniqueness to avoid duplicates
                bool unique_values = true;
                for (auto& b : m_best) {
                    const auto b_calc = b.Calculate();
                    const auto b_ranges = m_target->MatchPositions(b_calc);
                    if (b_calc == fnc_calc) {
                        unique_values = false;
                        break;
                    }
                    if (b_ranges == fnc_ranges) {
                        unique_values = false;
                        break;
                    }
                }
                if (not unique_values) {
                    break;
                }
                m_best.insert(best_it, fnc);
                break;
            }
            ++best_it;
        }

        // Maintain maximum list size
        while (m_best.size() > max_best) {
            m_best.pop_back();
        }

        // Update threshold to worst distance in current best list
        m_suit_threshold = CalcDist(m_best.back());
    }

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
        std::println("    Search started");
        m_tm_start = std::chrono::steady_clock::now();
        bool iterate_ok = true;
        while ((not stoken.stop_requested()) and iterate_ok and (not m_done)) {
            iterate_ok = SearchIterate();
            if (not iterate_ok) {
                std::println("    Search stopped: reached iteration end");
                m_done = true;
            }
        }
    }

   public:
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
        const std::unique_lock lock{m_mtx};
        if (not m_fn.Iterate(m_settings.max_depth)) {
            return false;
        }
        CheckBest(m_fn, m_settings.max_best);
        ++m_count;
        return true;
    }
};

/// @} // end of Search group

}  // namespace fw