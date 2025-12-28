#pragma once

#include <stdint.h>

#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace fw
{

/// @addtogroup Common
/// @{

/// @brief Distance type for comparing function outputs
using Distance = std::size_t;

/// @brief Type for serial numbers of function trees (supports very large numbers)
using SerialNumber_t = __int128;

/**
 * @class RangeSet
 * @brief Efficient representation of sets using ranges
 * @tparam Tnum Numeric type for range elements (must be integral)
 * 
 * Stores sets as collections of contiguous ranges [start, end].
 * Efficient for operations on large contiguous sets.
 * 
 * Example: {1,2,3,5,6,7,10} → ranges: [1,3], [5,7], [10,10]
 */
template <typename Tnum>
class RangeSet
{
   public:
    /**
     * @brief Add a single number to the set
     * @param number Number to add
     */
    void Add(Tnum number) { AddRange(number, number); }

    /**
     * @brief Add a range of numbers [start, end] to the set
     * @param start Start of range (inclusive)
     * @param end End of range (inclusive)
     * 
     * Automatically merges overlapping or adjacent ranges.
     */
    void AddRange(Tnum start, Tnum end)
    {
        if (start > end) {
            std::swap(start, end);
        }

        auto range_it = m_ranges.lower_bound({start, end});

        // Merge with previous range if overlapping or adjacent
        if (range_it != m_ranges.begin()) {
            auto prev = std::prev(range_it);
            if (prev->second >= start - 1) {
                start = prev->first;
                end = std::max(prev->second, end);
                m_ranges.erase(prev);
            }
        }

        // Merge with subsequent ranges
        while (range_it != m_ranges.end() && range_it->first <= end + 1) {
            end = std::max(end, range_it->second);
            range_it = m_ranges.erase(range_it);
        }

        m_ranges.emplace(start, end);
    }

    /**
     * @brief Count total number of elements in all ranges
     * @return Total count
     */
    [[nodiscard]] std::size_t Count() const
    {
        std::size_t total = 0;
        for (const auto& range : m_ranges) {
            total += range.second - range.first + 1;
        }
        return total;
    }

    /// @brief Equality comparison operator
    bool operator==(const RangeSet& other) const { return m_ranges == other.m_ranges; }

    /**
     * @brief Convert to string representation for debugging
     * @return String showing all ranges
     */
    [[nodiscard]] std::string Str() const
    {
        std::string str;
        for (const auto& range : m_ranges) {
            if (range.first != range.second) {
                str += std::format("[{},{}] ", range.first, range.second);
            }
            else {
                str += std::format("{} ", range.first);
            }
        }
        return str;
    }

   private:
    /// @brief Internal storage of ranges as [start, end] pairs, sorted by start
    std::set<std::pair<Tnum, Tnum>> m_ranges;
};

/// @} // end of Common group

}  // namespace fw