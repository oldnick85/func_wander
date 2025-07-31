#pragma once

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <utility>  // for std::pair
#include <vector>

namespace fw
{
using Distance = std::size_t;
using SerialNumber_t = __int128;

template <typename Tnum>
class RangeSet
{
   public:
    // Adds a single number to the set.
    void Add(Tnum number) { AddRange(number, number); }

    // Adds a range [start, end] to the set.
    void AddRange(Tnum start, Tnum end)
    {
        if (start > end) {
            std::swap(start, end);
        }

        auto it = m_ranges.lower_bound({start, end});

        // Merge with the previous range if overlapping or adjacent.
        if (it != m_ranges.begin()) {
            auto prev = std::prev(it);
            if (prev->second >= start - 1) {
                start = prev->first;
                end = std::max(prev->second, end);
                m_ranges.erase(prev);
            }
        }

        // Merge with subsequent ranges.
        while (it != m_ranges.end() && it->first <= end + 1) {
            end = std::max(end, it->second);
            it = m_ranges.erase(it);
        }

        m_ranges.emplace(start, end);
    }

    // Returns the total count of numbers in all ranges.
    std::size_t Count() const
    {
        std::size_t total = 0;
        for (const auto& range : m_ranges) {
            total += range.second - range.first + 1;
        }
        return total;
    }

    // Checks if two RangeSets are equal.
    bool operator==(const RangeSet& other) const
    {
        return m_ranges == other.m_ranges;
    }

    // Str all ranges for debugging.
    std::string Str() const
    {
        std::string str;
        for (const auto& range : m_ranges) {
            if (range.first != range.second)
                str += std::format("[{},{}] ", range.first, range.second);
            else
                str += std::format("{} ", range.first);
        }
        return str;
    }

   private:
    // Stores ranges as [start, end] pairs, sorted by start.
    std::set<std::pair<Tnum, Tnum>> m_ranges;
};

}  // namespace fw