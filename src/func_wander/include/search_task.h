#pragma once

#include <format>
#include <functional>
#include <list>
#include <mutex>
#include <stop_token>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "func_node.h"
#include "target.h"

namespace fw
{

struct Settings
{
    bool operator==(const Settings& other) const
    {
        return ((save_file == other.save_file) and
                (max_best == other.max_best) and
                (max_depth == other.max_depth));
    }

    std::string save_file;
    std::size_t max_best = 32;
    std::size_t max_depth = 3;
};

template <typename FuncValue_t, bool SKIP_CONSTANT = false,
          bool SKIP_SYMMETRIC = false>
class SearchTask
{
   public:
    using FN_t = FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;

    explicit SearchTask(const Settings& settings, AtomFuncs<FuncValue_t>* atoms,
                        Target<FuncValue_t>* target)
        : m_settings(settings), m_atoms(atoms), m_target(target), m_fn{atoms}
    {
    }

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

    bool Iterate() { return m_fn.Iterate(m_settings.max_depth); }

    void Run()
    {
        m_thread = std::jthread(std::bind_front(&SearchTask::Search, this));
    }

    void Stop()
    {
        m_thread.request_stop();
        m_thread.join();
    }

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

    bool Done() const { return m_done; }

    std::vector<FN_t> Best() const
    {
        std::unique_lock lock{m_mtx};
        return m_best;
    }

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
    Settings m_settings;
    AtomFuncs<FuncValue_t>* m_atoms = nullptr;
    Target<FuncValue_t>* m_target = nullptr;
    FN_t m_fn;
    std::chrono::time_point<std::chrono::steady_clock> m_tm_start;
    std::size_t m_count = 0;
    std::list<FN_t> m_best;
    std::size_t m_dist_threshold = 0;
    std::mutex m_mtx;
    std::jthread m_thread;
    std::atomic_bool m_done = false;

    std::size_t CalcDist(FN_t& fnc) const
    {
        const auto fnc_calc = fnc.Calculate();
        const auto fnc_cmp = m_target->Compare(fnc_calc);
        return fnc_cmp * 10 + fnc.CurrentMaxLevel() + fnc.FunctionsCount() * 2;
    }

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

        while (m_best.size() > max_best)
            m_best.pop_back();

        m_dist_threshold = CalcDist(m_best.back());
    }

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

}  // namespace fw