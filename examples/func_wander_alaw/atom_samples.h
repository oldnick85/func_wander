#pragma once

#include <bitset>

#include <atom.h>

using fw::AtomFunc0;
using fw::AtomFunc1;
using fw::AtomFunc2;

using Value_t = int16_t;

constexpr Value_t VALUE_FIRST = 0;
constexpr Value_t VALUE_LAST = 255;
constexpr std::size_t VALUES_COUNT = VALUE_LAST + 1;

class AF_CONST : public AtomFunc0<Value_t>
{
   public:
    explicit AF_CONST(Value_t val) : m_val(val)
    {
        m_values.reserve(VALUES_COUNT);
        for (Value_t i = {}; std::cmp_less(i, VALUES_COUNT); ++i) {
            m_values.push_back(m_val);
        }
    }

    ~AF_CONST() override = default;

    [[nodiscard]] const FuncValues_t& Calculate() const override { return m_values; }

    [[nodiscard]] bool Constant() const override { return true; }

    [[nodiscard]] std::string Str() const override { return std::to_string(m_val); }

   private:
    Value_t m_val = 0;
    FuncValues_t m_values;
};

class AF_ARG_X : public AtomFunc0<Value_t>
{
   public:
    AF_ARG_X()
    {
        m_values.reserve(VALUES_COUNT);
        for (Value_t i = {}; std::cmp_less(i, VALUES_COUNT); ++i) {
            m_values.push_back(i);
        }
    }

    ~AF_ARG_X() override = default;

    [[nodiscard]] const FuncValues_t& Calculate() const override { return m_values; }

    [[nodiscard]] bool Constant() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "X"; }

   private:
    FuncValues_t m_values;
};

class AF_FW1 : public AtomFunc1<Value_t>
{
   public:
    ~AF_FW1() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>((arg[i] << 4) + 8));
        }
        return res;
    }

    [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "FW1"; }
};

class AF_FW2 : public AtomFunc1<Value_t>
{
   public:
    ~AF_FW2() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            // NOLINTNEXTLINE(readability-magic-numbers)
            res.push_back(static_cast<Value_t>(((127 - arg[i]) << 4) + 8));
        }
        return res;
    }

    [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "FW2"; }
};

class AF_NOT : public AtomFunc1<Value_t>
{
   public:
    ~AF_NOT() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(~arg[i]));
        }
        return res;
    }

    [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "NOT"; }
};

class AF_BITCOUNT : public AtomFunc1<Value_t>
{
   public:
    ~AF_BITCOUNT() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(std::bitset<16>{static_cast<uint16_t>(arg[i])}.count()));
        }
        return res;
    }

    [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "BITCOUNT"; }
};

class AF_BITCLZ : public AtomFunc1<Value_t>
{
   public:
    ~AF_BITCLZ() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(__builtin_clz(arg[i]) - 16));
        }
        return res;
    }

    [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "BITCLZ"; }
};

class AF_SUM : public AtomFunc2<Value_t>
{
   public:
    ~AF_SUM() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] + arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return true; }

    [[nodiscard]] bool Idempotent() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "SUM"; }
};

class AF_SUB : public AtomFunc2<Value_t>
{
   public:
    ~AF_SUB() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] - arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return false; }

    [[nodiscard]] bool Idempotent() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "SUB"; }
};

class AF_AND : public AtomFunc2<Value_t>
{
   public:
    ~AF_AND() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] & arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return true; }

    [[nodiscard]] bool Idempotent() const override { return true; }

    [[nodiscard]] std::string Str() const override { return "AND"; }
};

class AF_OR : public AtomFunc2<Value_t>
{
   public:
    ~AF_OR() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] | arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return true; }

    [[nodiscard]] bool Idempotent() const override { return true; }

    [[nodiscard]] std::string Str() const override { return "OR"; }
};

class AF_XOR : public AtomFunc2<Value_t>
{
   public:
    ~AF_XOR() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] ^ arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return true; }

    [[nodiscard]] bool Idempotent() const override { return true; }

    [[nodiscard]] std::string Str() const override { return "XOR"; }
};

class AF_SHR : public AtomFunc2<Value_t>
{
   public:
    ~AF_SHR() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] >> arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return false; }

    [[nodiscard]] bool Idempotent() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "SHR"; }
};

class AF_SHL : public AtomFunc2<Value_t>
{
   public:
    ~AF_SHL() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_COUNT);
        assert(arg2.size() == VALUES_COUNT);
        FuncValues_t res;
        res.reserve(VALUES_COUNT);
        for (std::size_t i = {}; i < VALUES_COUNT; ++i) {
            res.push_back(static_cast<Value_t>(arg1[i] << arg2[i]));
        }
        return res;
    }

    [[nodiscard]] bool Commutative() const override { return false; }

    [[nodiscard]] bool Idempotent() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "SHL"; }
};