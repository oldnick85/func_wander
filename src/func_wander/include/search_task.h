#pragma once

#include <format>
#include <functional>
#include <list>
#include <mutex>
#include <stop_token>
#include <thread>
#include <atomic>
#include <chrono>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "func_node.h"
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
        return ((save_file == other.save_file) and
                (max_best == other.max_best) and
                (max_depth == other.max_depth));
    }

    std::string save_file;      ///< 📁 File path for automatic save/load of search state
    std::size_t max_best = 32;  ///< 🏆 Maximum number of best functions to retain
    std::size_t max_depth = 3;  ///< 🌳 Maximum depth of function trees to explore
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
template <typename FuncValue_t, bool SKIP_CONSTANT = false,
          bool SKIP_SYMMETRIC = false>
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
    explicit SearchTask(const Settings& settings, AtomFuncs<FuncValue_t>* atoms,
                        Target<FuncValue_t>* target)
        : m_settings(settings), m_atoms(atoms), m_target(target), m_fn{atoms}
    {
    }

    /**
     * @brief Equality comparison operator
     * @param other SearchTask to compare with
     * @return true if tasks have identical state
     */
    bool operator==(const SearchTask<FuncValue_t, SKIP_CONSTANT,
                                     SKIP_SYMMETRIC>& other) const
    {
        if (m_settings != other.m_settings)
            return false;
        if (m_atoms != other.m_atoms)
            return false;
        if (m_target != other.m_target)
            return false;
        if (m_fn != other.m_fn)
            return false;
        if (m_count != other.m_count)
            return false;
        if (m_best != other.m_best)
            return false;
        if (m_dist_threshold != other.m_dist_threshold)
            return false;
        if (m_done != other.m_done)
            return false;
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
    void Run()
    {
        m_thread = std::jthread(std::bind_front(&SearchTask::Search, this));
    }

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
    json ToJSON() const
    {
        json j;
        j["settings"]["max_best"] = m_settings.max_best;
        j["settings"]["max_depth"] = m_settings.max_depth;
        j["count"] = m_count;
        j["done"] = m_done.load();
        j["dist_threshold"] = m_dist_threshold;
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
        if (j.is_discarded())
            return false;

        if (not j.is_object())
            return false;

        const auto j_settings = j.find("settings");
        if (j_settings == j.end())
            return false;
        if (not j_settings->is_object())
            return false;

        const auto j_settings_max_best = j_settings->find("max_best");
        if (j_settings_max_best == j_settings->end())
            return false;
        if (not j_settings_max_best->is_number())
            return false;
        m_settings.max_best = j_settings_max_best->get<std::size_t>();

        const auto j_settings_max_depth = j_settings->find("max_depth");
        if (j_settings_max_depth == j_settings->end())
            return false;
        if (not j_settings_max_depth->is_number())
            return false;
        m_settings.max_depth = j_settings_max_depth->get<std::size_t>();

        const auto j_count = j.find("count");
        if (j_count == j.end())
            return false;
        if (not j_count->is_number())
            return false;
        m_count = j_count->get<std::size_t>();

        const auto j_done = j.find("done");
        if (j_done == j.end())
            return false;
        if (not j_done->is_boolean())
            return false;
        m_done = j_done->get<bool>();

        const auto j_dist_threshold = j.find("dist_threshold");
        if (j_dist_threshold == j.end())
            return false;
        if (not j_dist_threshold->is_number())
            return false;
        m_dist_threshold = j_dist_threshold->get<std::size_t>();

        const auto j_fn = j.find("current_fn");
        if (j_fn == j.end())
            return false;
        if (not j_fn->is_object())
            return false;

        if (not m_fn.FromJSON(*j_fn))
            return false;

        m_best.clear();
        const auto j_best = j.find("best");
        if (j_best != j.end()) {
            if (not j_best->is_array())
                return false;

            for (auto it = j_best->begin(); it != j_best->end(); ++it) {
                m_best.emplace_back(m_atoms);
                if (not m_best.back().FromJSON(*it))
                    return false;
            }
        }

        return true;
    }

    /**
     * @brief Check if search has completed
     * @return true if search exhausted all possibilities, false otherwise
     */
    bool Done() const { return m_done; }

    /**
     * @brief Get current best functions found
     * @return Vector of best function trees, sorted by quality
     * 
     * Functions are ranked by a composite score considering:
     * 1. Distance to target (accuracy)
     * 2. Tree depth (simplicity)
     * 3. Node count (compactness)
     */
    std::vector<FN_t> Best() const
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
        std::unique_lock lock{m_mtx};
        const auto sn = m_fn.SerialNumber();
        const auto max_sn = m_fn.MaxSerialNumber(m_settings.max_depth);
        const float done_percent = (sn * 100.0f) / max_sn;
        const auto d = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - m_tm_start)
                           .count();
        const float c_per_sec = static_cast<float>(m_count) * 1000.0f / d;
        auto status = std::format(
            "iteration {}({:3}/{:3}): {} ({}%)\n{} iterations per second\n",
            m_count, float(sn), float(max_sn), m_fn.Repr(), done_percent,
            c_per_sec);
        for (auto& b : m_best) {
            status += std::format(
                "{}({}): {} <{}>\n", m_target->Compare(b.Calculate()),
                b.CurrentMaxLevel(), b.Repr(),
                m_target->MatchPositions(b.Calculate()).Str());
        }
        return status;
    }

private:
    Settings m_settings;                       ///< ⚙️ Search configuration parameters
    AtomFuncs<FuncValue_t>* m_atoms = nullptr; ///< 🧩 Reference to atomic function library
    Target<FuncValue_t>* m_target = nullptr;   ///< 🎯 Reference to target specification
    FN_t m_fn;                                 ///< 🌳 Current function being evaluated
    std::chrono::time_point<std::chrono::steady_clock> m_tm_start; ///< ⏱️ Search start time
    std::size_t m_count = 0;                   ///< 🔢 Number of iterations performed
    std::list<FN_t> m_best;                    ///< 🏆 Best functions found (maintained in order)
    std::size_t m_dist_threshold = 0;          ///< 📊 Worst distance currently in best list
    std::mutex m_mtx;                          ///< 🔐 Mutex for thread-safe state access
    std::jthread m_thread;                     ///< 🧵 Background search thread
    std::atomic_bool m_done = false;           ///< ✅ Completion flag (atomic for thread safety)

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
    std::size_t CalcDist(FN_t& fnc) const
    {
        const auto fnc_calc = fnc.Calculate();
        const auto fnc_cmp = m_target->Compare(fnc_calc);
        return fnc_cmp * 10 + fnc.CurrentMaxLevel() + fnc.FunctionsCount() * 2;
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
            if (new_dist > m_dist_threshold)
                return;
        }

        auto it = m_best.begin();
        while (it != m_best.end()) {
            const auto dist = CalcDist(*it);
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
                if (not unique_values)
                    break;
                m_best.insert(it, fnc);
                break;
            }
            ++it;
        }

        // Maintain maximum list size
        while (m_best.size() > max_best)
            m_best.pop_back();

        // Update threshold to worst distance in current best list
        m_dist_threshold = CalcDist(m_best.back());
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
        std::unique_lock lock{m_mtx};
        if (not m_fn.Iterate(m_settings.max_depth))
            return false;
        CheckBest(m_fn, m_settings.max_best);
        ++m_count;
        return true;
    }
};

/// @} // end of Search group

}  // namespace fw