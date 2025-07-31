#pragma once

#include "common.h"

namespace fw
{

template <typename FuncValue_t>
class Target
{
   public:
    using FuncValues_t = std::vector<FuncValue_t>;

    virtual ~Target() = default;

    virtual Distance Compare(const FuncValues_t& values) const = 0;
    virtual RangeSet<std::size_t> MatchPositions(
        const FuncValues_t& values) const = 0;
    virtual FuncValues_t Values() const = 0;
};

}  // namespace fw