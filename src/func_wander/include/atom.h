#pragma once

#include <string>
#include <variant>
#include <vector>

#include "common.h"

namespace fw
{

/**
 * @defgroup Atoms Atomic Functions
 * @brief Base classes for atomic functions used in function synthesis
 * @{
 */

/**
 * @brief Abstract base class for all atomic function representations
 * 
 * Provides a common interface for string representation of functions.
 * All atomic function types (nullary, unary, binary) inherit from this class.
 */
class AtomFuncBase
{
   public:
    virtual ~AtomFuncBase() = default;

    /**
     * @brief Get string representation of the function
     * @return String representation (e.g., "sin", "+", "const_5")
     */
    [[nodiscard]] virtual std::string Str() const = 0;
};

/**
 * @brief Base class for nullary functions (constants/functions with no arguments)
 * @tparam FuncValue_t Type of function values (e.g., int, double, bool)
 * 
 * Represents functions that take no arguments and produce a constant value.
 * Used for terminal nodes in function trees.
 */
template <typename FuncValue_t>
class AtomFunc0 : public AtomFuncBase
{
   public:
    /// Vector type for function values
    using FuncValues_t = std::vector<FuncValue_t>;

    ~AtomFunc0() override = default;

    /**
     * @brief Calculate the constant values
     * @return Vector of constant values
     */
    [[nodiscard]] virtual const FuncValues_t& Calculate() const = 0;

    /**
     * @brief Check if function is constant
     * @return true if the function always returns the same value
     */
    [[nodiscard]] virtual bool Constant() const = 0;
};

/**
 * @brief Base class for unary functions (functions with one argument)
 * @tparam FuncValue_t Type of function values
 * 
 * Represents functions that take one argument (e.g., sin(x), not(x), sqrt(x)).
 */
template <typename FuncValue_t>
class AtomFunc1 : public AtomFuncBase
{
   public:
    /// Vector type for function values
    using FuncValues_t = std::vector<FuncValue_t>;

    ~AtomFunc1() override = default;

    /**
     * @brief Calculate function values for given argument
     * @param arg Vector of argument values
     * @return Vector of resulting values
     */
    [[nodiscard]] virtual FuncValues_t Calculate(const FuncValues_t& arg) const = 0;

    /**
     * @brief Check if function is involutive (self-inverse)
     * @return true if f(f(x)) = x for all x
     */
    [[nodiscard]] virtual bool Involutive() const = 0;

    /**
     * @brief Check if function simply returns its argument
     * @return true if f(x) = x
     */
    [[nodiscard]] virtual bool Argument() const = 0;
};

/**
 * @brief Base class for binary functions (functions with two arguments)
 * @tparam FuncValue_t Type of function values
 * 
 * Represents functions that take two arguments (e.g., x+y, x*y, max(x,y)).
 */
template <typename FuncValue_t>
class AtomFunc2 : public AtomFuncBase
{
   public:
    /// Vector type for function values
    using FuncValues_t = std::vector<FuncValue_t>;

    ~AtomFunc2() override = default;

    /**
     * @brief Calculate function values for given arguments
     * @param arg1 Vector of first argument values
     * @param arg2 Vector of second argument values
     * @return Vector of resulting values
     */
    [[nodiscard]] virtual FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const = 0;

    /**
     * @brief Check if function is commutative
     * @return true if f(x,y) = f(y,x) for all x,y
     */
    [[nodiscard]] virtual bool Commutative() const = 0;

    /**
     * @brief Check if function is idempotent
     * @return true if f(x,x) = x for all x
     */
    [[nodiscard]] virtual bool Idempotent() const = 0;
};

/**
 * @brief Type alias for polymorphic function container
 * @tparam FuncValue_t Type of function values
 * 
 * Stores pointers to atomic functions of any arity (0, 1, or 2).
 * Used for runtime polymorphism of function objects.
 */
template <typename FuncValue_t>
using AtomFunc = std::variant<AtomFunc0<FuncValue_t>*, AtomFunc1<FuncValue_t>*, AtomFunc2<FuncValue_t>*>;

/** @} */  // end of Atoms group

}  // namespace fw