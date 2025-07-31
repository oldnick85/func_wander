#pragma once

#include <bitset>

#include <atom.h>

using namespace fw;

constexpr std::size_t VALUES_RANGE = 256;
using Value_t = int16_t;

class AF_CONST : public AtomFunc0<Value_t>
{
   public:
    AF_CONST(Value_t val) : m_val(val)
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
    Value_t m_val = 0;
    FuncValues_t m_values;
};

class AF_ARG_X : public AtomFunc0<Value_t>
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

class AF_FW1 : public AtomFunc1<Value_t>
{
   public:
    virtual ~AF_FW1() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg) const
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back((arg[i] << 4) + 8);
        return res;
    }

    virtual bool Involutive() const { return true; }
    virtual bool Argument() const { return false; }

    virtual std::string Str() const { return "FW1"; }
};

class AF_FW2 : public AtomFunc1<Value_t>
{
   public:
    virtual ~AF_FW2() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg) const
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(((127 - arg[i]) << 4) + 8);
        return res;
    }

    virtual bool Involutive() const { return true; }
    virtual bool Argument() const { return false; }

    virtual std::string Str() const { return "FW2"; }
};

class AF_NOT : public AtomFunc1<Value_t>
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

class AF_BITCOUNT : public AtomFunc1<Value_t>
{
   public:
    virtual ~AF_BITCOUNT() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg) const
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(
                std::bitset<16>{static_cast<uint16_t>(arg[i])}.count());
        return res;
    }

    virtual bool Involutive() const { return true; }
    virtual bool Argument() const { return false; }

    virtual std::string Str() const { return "BITCOUNT"; }
};

class AF_BITCLZ : public AtomFunc1<Value_t>
{
   public:
    virtual ~AF_BITCLZ() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg) const
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(__builtin_clz(arg[i]) - 16);
        return res;
    }

    virtual bool Involutive() const { return true; }
    virtual bool Argument() const { return false; }

    virtual std::string Str() const { return "BITCLZ"; }
};

class AF_SUM : public AtomFunc2<Value_t>
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

class AF_SUB : public AtomFunc2<Value_t>
{
   public:
    virtual ~AF_SUB() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] - arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return false; }

    virtual bool Idempotent() const { return false; }

    virtual std::string Str() const { return "SUB"; }
};

class AF_AND : public AtomFunc2<Value_t>
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

class AF_OR : public AtomFunc2<Value_t>
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

class AF_XOR : public AtomFunc2<Value_t>
{
   public:
    virtual ~AF_XOR() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] ^ arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return true; }

    virtual bool Idempotent() const { return true; }

    virtual std::string Str() const { return "XOR"; }
};

class AF_SHR : public AtomFunc2<Value_t>
{
   public:
    virtual ~AF_SHR() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] >> arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return false; }

    virtual bool Idempotent() const { return false; }

    virtual std::string Str() const { return "SHR"; }
};

class AF_SHL : public AtomFunc2<Value_t>
{
   public:
    virtual ~AF_SHL() = default;

    virtual FuncValues_t Calculate(const FuncValues_t& arg1,
                                   const FuncValues_t& arg2) const
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i)
            res.push_back(arg1[i] << arg2[i]);
        return res;
    }

    virtual bool Commutative() const { return false; }

    virtual bool Idempotent() const { return false; }

    virtual std::string Str() const { return "SHL"; }
};