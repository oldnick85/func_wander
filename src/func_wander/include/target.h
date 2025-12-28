#pragma once

#include "common.h"

namespace fw
{

/// @addtogroup Targets
/// @{

/**
 * @class Target
 * @brief Abstract base class for target function specifications
 * @tparam FuncValue_t Type of function values
 * 
 * Defines the target that synthesized functions should approximate.
 * Provides methods to compare candidate functions with the target.
 */
template <typename FuncValue_t>
class Target
{
   public:
    /// Vector type for function values
    using FuncValues_t = std::vector<FuncValue_t>;

    virtual ~Target() = default;

    /**
     * @brief Compare candidate function outputs with target
     * @param values Output values from candidate function
     * @return Distance metric (0 = perfect match, higher = worse)
     */
    [[nodiscard]] virtual Distance Compare(const FuncValues_t& values) const = 0;

    /**
     * @brief Find positions where candidate matches target
     * @param values Output values from candidate function
     * @return RangeSet of indices where values match target
     */
    [[nodiscard]] virtual RangeSet<std::size_t> MatchPositions(const FuncValues_t& values) const = 0;

    /**
     * @brief Get the target function values
     * @return Vector of desired output values
     */
    [[nodiscard]] virtual FuncValues_t Values() const = 0;
};

/// @} // end of Targets group

}  // namespace fw