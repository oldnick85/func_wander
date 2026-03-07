#pragma once

#include <compare>
#include <cstddef>
#include <unordered_set>

#include "common.h"
#include "func_node.h"
#include "target.h"

namespace fw
{

class SuitabilityMetrics
{
   public:
    SuitabilityMetrics() = default;

    SuitabilityMetrics(Distance distance, std::size_t max_level, std::size_t functions_count,
                       std::size_t functions_unique)
        : m_distance(distance),
          m_max_level(max_level),
          m_functions_count(functions_count),
          m_functions_unique(functions_unique)
    {
    }

    Distance distance() const { return m_distance; }
    std::size_t max_level() const { return m_max_level; }
    std::size_t functions_count() const { return m_functions_count; }
    std::size_t functions_unique() const { return m_functions_unique; }

   private:
    Distance m_distance = 1000000;
    std::size_t m_max_level;
    std::size_t m_functions_count;
    std::size_t m_functions_unique;

   public:
    bool operator==(const SuitabilityMetrics& other) const noexcept
    {
        if (m_distance != other.m_distance)
            return false;

        if (m_max_level != other.m_max_level)
            return false;

        if (m_functions_count != other.m_functions_count)
            return false;

        if (m_functions_unique != other.m_functions_unique)
            return false;

        return true;
    }

    std::strong_ordering operator<=>(const SuitabilityMetrics& other) const noexcept
    {
        if (m_distance > other.m_distance)
            return std::strong_ordering::greater;
        if (m_distance < other.m_distance)
            return std::strong_ordering::less;

        if (m_max_level > other.m_max_level)
            return std::strong_ordering::greater;
        if (m_max_level < other.m_max_level)
            return std::strong_ordering::less;

        //if (m_functions_count > other.m_functions_count)
        //    return std::strong_ordering::greater;
        //if (m_functions_count < other.m_functions_count)
        //    return std::strong_ordering::less;

        if (m_functions_unique > other.m_functions_unique)
            return std::strong_ordering::greater;
        if (m_functions_unique < other.m_functions_unique)
            return std::strong_ordering::less;

        return std::strong_ordering::equivalent;
    }
};

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
template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
SuitabilityMetrics CalcDist(FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& fnc, Target<FuncValue_t>* target)
{
    const auto fnc_calc = fnc.Calculate();
    const auto fnc_cmp = target->Compare(fnc_calc);
    std::unordered_set<SerialNumber_t, SerialNumberHash> uniqs{};
    fnc.UniqFunctionsSerialNumbers(uniqs);
    return SuitabilityMetrics(fnc_cmp, fnc.CurrentMaxLevel(), fnc.FunctionsCount(), uniqs.size());
}

}  // namespace fw