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

struct SerialNumberHash
{
    std::size_t operator()(__int128 x) const
    {
        uint64_t low = static_cast<uint64_t>(x);
        uint64_t high = static_cast<uint64_t>(x >> 64);
        return std::hash<uint64_t>{}(low) ^ (std::hash<uint64_t>{}(high) << 1);
    }
};

struct SerialNumberEqual
{
    bool operator()(__int128 a, __int128 b) const { return a == b; }
};

template <class T>
std::string format_with_si_prefix(T value)
{
    static constexpr std::array<const char*, 5> prefixes = {"", "k", "M", "G", "T"};

    if (value == 0) {
        return "0.000";
    }

    T divisor = 1;
    int prefix_index = 0;

    while (value >= static_cast<T>(1000) * divisor && prefix_index < 4) {
        divisor *= 1000;
        prefix_index++;
    }

    T integer_part = value / divisor;
    T remainder = value % divisor;

    // Calculate fractional part
    T fractional = remainder * 1000 / divisor;
    if (fractional > 999)
        fractional = 999;

    // Build result string
    std::string result;

    // Convert integer part to string
    T temp_int = integer_part;
    do {
        char digit = '0' + static_cast<char>(temp_int % 10);
        result.insert(result.begin(), digit);
        temp_int /= 10;
    } while (temp_int != 0);

    // Add decimal point
    result.push_back('.');

    // Add three-digit fractional part
    int frac_part = static_cast<int>(fractional);
    char hundreds = '0' + (frac_part / 100);
    char tens = '0' + ((frac_part % 100) / 10);
    char units = '0' + (frac_part % 10);

    result.push_back(hundreds);
    result.push_back(tens);
    result.push_back(units);

    result += prefixes[prefix_index];
    return result;
}

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