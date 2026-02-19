#pragma once

#include <chrono>

#include "common.h"
#include "comparison.h"

namespace fw
{
namespace status
{

struct BestFunc
{
    SuitabilityMetrics suit;
    std::string function;
    std::string match_positions;
};

struct Status
{
    SerialNumber_t snum{};
    SerialNumber_t max_sn{};
    float done_percent{};
    std::chrono::duration<int64_t, std::nano> elapsed{};
    std::chrono::duration<int64_t, std::nano> remaining{};
    std::size_t iterations_per_sec{};
    std::size_t sn_per_sec{};
    std::size_t iterations_count{};
    std::string current_function;
    std::vector<BestFunc> best_functions;

    std::string to_string() const
    {
        const auto remaining_h = std::chrono::duration_cast<std::chrono::hours>(remaining);
        const auto remaining_m = std::chrono::duration_cast<std::chrono::minutes>(remaining % std::chrono::hours(1));
        const auto remaining_s = std::chrono::duration_cast<std::chrono::seconds>(remaining % std::chrono::minutes(1));

        const auto elapsed_h = std::chrono::duration_cast<std::chrono::hours>(elapsed);
        const auto elapsed_m = std::chrono::duration_cast<std::chrono::minutes>(elapsed % std::chrono::hours(1));
        const auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsed % std::chrono::minutes(1));

        auto str = std::format(
            "iteration {}; func sn {} from max {}; progress {}%; speed {} ips; elapsed: {}:{:02d}:{:02d}; remaining: "
            "{}:{:02d}:{:02d}; function {}\n",
            iterations_count, format_with_si_prefix(snum), format_with_si_prefix(max_sn), done_percent,
            format_with_si_prefix(iterations_per_sec), elapsed_h.count(), elapsed_m.count(), elapsed_s.count(),
            remaining_h.count(), remaining_m.count(), remaining_s.count(), current_function);

        str += std::format("|  dist  | lvl | fnc | fnu | {:48}| coincidences\n", "function");
        for (auto& best : best_functions) {
            str += std::format("| {:6} | {:3} | {:3} | {:3} | {:48}| {} \n", best.suit.distance(),
                               best.suit.max_level(), best.suit.functions_count(), best.suit.functions_unique(),
                               best.function, best.match_positions);
        }
        return str;
    }
};

}  // namespace status
}  // namespace fw