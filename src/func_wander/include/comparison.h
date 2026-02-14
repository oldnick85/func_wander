#pragma once

#include <compare>
#include <cstddef>

#include "common.h"

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

}  // namespace fw