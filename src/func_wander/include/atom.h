#pragma once

#include <string>
#include <variant>

#include "common.h"

namespace fw
{

class AtomFuncBase
{
   public:
    virtual ~AtomFuncBase() = default;
    virtual std::string Str() const = 0;
};

template <typename FuncValue_t>
class AtomFunc0 : public AtomFuncBase
{
   public:
    using FuncValues_t = std::vector<FuncValue_t>;

    virtual ~AtomFunc0() = default;
    virtual const FuncValues_t& Calculate() const = 0;
    virtual bool Constant() const = 0;
};

template <typename FuncValue_t>
class AtomFunc1 : public AtomFuncBase
{
   public:
    using FuncValues_t = std::vector<FuncValue_t>;

    virtual ~AtomFunc1() = default;
    virtual FuncValues_t Calculate(const FuncValues_t& arg) const = 0;
    virtual bool Involutive() const = 0;
    virtual bool Argument() const = 0;
};

template <typename FuncValue_t>
class AtomFunc2 : public AtomFuncBase
{
   public:
    using FuncValues_t = std::vector<FuncValue_t>;

    virtual ~AtomFunc2() = default;
    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const = 0;
    virtual bool Commutative() const = 0;
    virtual bool Idempotent() const = 0;
};

template <typename FuncValue_t>
using AtomFunc = std::variant<AtomFunc0<FuncValue_t>*, AtomFunc1<FuncValue_t>*,
                              AtomFunc2<FuncValue_t>*>;
}  // namespace fw