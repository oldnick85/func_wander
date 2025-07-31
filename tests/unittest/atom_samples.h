#pragma once

#include <bitset>

#include <atom.h>

using namespace fw;

constexpr std::size_t VALUES_RANGE = 256;

class AF_CONST : public AtomFunc0<uint16_t>
{
   public:
    AF_CONST(uint16_t val) : m_val(val)
    {
        m_values.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            m_values.push_back(m_val);
    }

    virtual ~AF_CONST() = default;

    virtual const FuncValues_t& Calculate() const { return m_values; }

    virtual bool Constant() const { return true; }

    virtual std::string Str() const { return std::to_string(m_val); }

   private:
    uint16_t m_val = 0;
    FuncValues_t m_values;
};

class AF_ARG_X : public AtomFunc0<uint16_t>
{
   public:
    AF_ARG_X()
    {
        m_values.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            m_values.push_back(i);
    }

    virtual ~AF_ARG_X() = default;

    virtual const FuncValues_t& Calculate() const { return m_values; }

    virtual bool Constant() const { return false; }

    virtual std::string Str() const { return "X"; }

   private:
    FuncValues_t m_values;
};

class AF_NOT : public AtomFunc1<uint16_t>
{
   public:
    virtual ~AF_NOT() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg) const
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(~arg[i]);
        return res;
    }

    virtual bool Involutive() const { return true; }
    virtual bool Argument() const { return false; }

    virtual std::string Str() const { return "NOT"; }
};

class AF_BITCOUNT : public AtomFunc1<uint16_t>
{
   public:
    virtual ~AF_BITCOUNT() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg) const
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(std::bitset<16>{arg[i]}.count());
        return res;
    }

    virtual bool Involutive() const { return true; }
    virtual bool Argument() const { return false; }

    virtual std::string Str() const { return "BITCOUNT"; }
};

class AF_SUM : public AtomFunc2<uint16_t>
{
   public:
    virtual ~AF_SUM() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] + arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return true; }
    virtual bool Idempotent() const { return false; }

    virtual std::string Str() const { return "SUM"; }
};

class AF_AND : public AtomFunc2<uint16_t>
{
   public:
    virtual ~AF_AND() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] & arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return true; }

    virtual bool Idempotent() const { return true; }

    virtual std::string Str() const { return "AND"; }
};

class AF_OR : public AtomFunc2<uint16_t>
{
   public:
    virtual ~AF_OR() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] | arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return true; }

    virtual bool Idempotent() const { return true; }

    virtual std::string Str() const { return "OR"; }
};