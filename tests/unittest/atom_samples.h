#pragma once

#include <bitset>

#include <atom.h>

using fw::AtomFunc0;
using fw::AtomFunc1;
using fw::AtomFunc2;
using fw::Characteristics;

constexpr std::size_t VALUES_RANGE = 256;

class AF_CONST : public AtomFunc0<uint16_t>
{
   public:
    explicit AF_CONST(uint16_t val) : m_val(val)
    {
        m_values.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            m_values.push_back(m_val);
        }
        m_chars.min = m_val;
        m_chars.max = m_val;
    }

    ~AF_CONST() override = default;

    [[nodiscard]] const FuncValues_t& Calculate() const override { return m_values; }

    [[nodiscard]] const Characteristics<uint16_t>& Chars() const override { return m_chars; }

    [[nodiscard]] bool Constant() const override { return true; }

    [[nodiscard]] std::string Str() const override { return std::to_string(m_val); }

   private:
    uint16_t m_val = 0;
    FuncValues_t m_values;
    Characteristics<uint16_t> m_chars;
};

class AF_ARG_X : public AtomFunc0<uint16_t>
{
   public:
    AF_ARG_X()
    {
        m_values.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            m_values.push_back(i);
        }
        m_chars.min = 0;
        m_chars.max = VALUES_RANGE - 1;
    }

    ~AF_ARG_X() override = default;

    [[nodiscard]] const FuncValues_t& Calculate() const override { return m_values; }

    [[nodiscard]] const Characteristics<uint16_t>& Chars() const override { return m_chars; }

    [[nodiscard]] bool Constant() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "X"; }

   private:
    FuncValues_t m_values;
    Characteristics<uint16_t> m_chars;
};

class AF_NOT : public AtomFunc1<uint16_t>
{
   public:
    ~AF_NOT() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            res.push_back(~arg[i]);
        }
        return res;
    }

    [[nodiscard]] bool CheckChars([[maybe_unused]] const Characteristics<uint16_t>& arg_chars) const override
    {
        return true;
    }

    [[nodiscard]] [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "NOT"; }
};

class AF_BITCOUNT : public AtomFunc1<uint16_t>
{
   public:
    ~AF_BITCOUNT() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg) const override
    {
        assert(arg.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            res.push_back(std::bitset<16>{arg[i]}.count());
        }
        return res;
    }

    [[nodiscard]] bool CheckChars([[maybe_unused]] const Characteristics<uint16_t>& arg_chars) const override
    {
        return true;
    }

    [[nodiscard]] bool Involutive() const override { return true; }
    [[nodiscard]] bool Argument() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "BITCOUNT"; }
};

class AF_SUM : public AtomFunc2<uint16_t>
{
   public:
    ~AF_SUM() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            res.push_back(arg1[i] + arg2[i]);
        }
        return res;
    }

    [[nodiscard]] bool CheckChars([[maybe_unused]] const Characteristics<uint16_t>& arg1_chars,
                                  [[maybe_unused]] const Characteristics<uint16_t>& arg2_chars) const override
    {
        return true;
    }

    [[nodiscard]] bool Commutative() const override { return true; }
    [[nodiscard]] bool Idempotent() const override { return false; }

    [[nodiscard]] std::string Str() const override { return "SUM"; }
};

class AF_AND : public AtomFunc2<uint16_t>
{
   public:
    ~AF_AND() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            res.push_back(arg1[i] & arg2[i]);
        }
        return res;
    }

    [[nodiscard]] bool CheckChars([[maybe_unused]] const Characteristics<uint16_t>& arg1_chars,
                                  [[maybe_unused]] const Characteristics<uint16_t>& arg2_chars) const override
    {
        return true;
    }

    [[nodiscard]] bool Commutative() const override { return true; }

    [[nodiscard]] bool Idempotent() const override { return true; }

    [[nodiscard]] std::string Str() const override { return "AND"; }
};

class AF_OR : public AtomFunc2<uint16_t>
{
   public:
    ~AF_OR() override = default;

    [[nodiscard]] FuncValues_t Calculate(const FuncValues_t& arg1, const FuncValues_t& arg2) const override
    {
        assert(arg1.size() == VALUES_RANGE);
        assert(arg2.size() == VALUES_RANGE);
        FuncValues_t res;
        res.reserve(VALUES_RANGE);
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            res.push_back(arg1[i] | arg2[i]);
        }
        return res;
    }

    [[nodiscard]] bool CheckChars([[maybe_unused]] const Characteristics<uint16_t>& arg1_chars,
                                  [[maybe_unused]] const Characteristics<uint16_t>& arg2_chars) const override
    {
        return true;
    }

    [[nodiscard]] bool Commutative() const override { return true; }

    [[nodiscard]] bool Idempotent() const override { return true; }

    [[nodiscard]] std::string Str() const override { return "OR"; }
};