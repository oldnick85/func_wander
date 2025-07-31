#pragma once

#include <format>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "atom.h"

namespace fw
{

struct AtomIndex
{
    bool operator==(const AtomIndex& other) const
    {
        return ((arity == other.arity) and (num == other.num));
    }

    std::size_t arity = 0;
    std::size_t num = 0;
};

template <typename FuncValue_t>
class AtomFuncs
{
   public:
    void Add(AtomFunc0<FuncValue_t>* func)
    {
        if (func->Constant()) {
            arg0.push_back(func);
        }
        else {
            arg0.insert(arg0.begin(), func);
        }
    }

    void Add(AtomFunc1<FuncValue_t>* func) { arg1.push_back(func); }

    void Add(AtomFunc2<FuncValue_t>* func) { arg2.push_back(func); }

    AtomFuncBase* Get(std::size_t arity, std::size_t num)
    {
        switch (arity) {
            case 0:
                return arg0[num];
            case 1:
                return arg1[num];
            case 2:
                return arg2[num];
            default:
                return nullptr;
        }
        return nullptr;
    }

    std::vector<AtomFunc0<FuncValue_t>*> arg0;  ///< constants at the end
    std::vector<AtomFunc1<FuncValue_t>*> arg1;
    std::vector<AtomFunc2<FuncValue_t>*> arg2;
};

template <typename FuncValue_t, bool SKIP_CONSTANT = false,
          bool SKIP_SYMMETRIC = false>
class FuncNode
{
   public:
    using FuncValues_t = std::vector<FuncValue_t>;
    using AtomFuncs_t = AtomFuncs<FuncValue_t>;

    FuncNode(AtomFuncs_t* atoms) : m_atoms(atoms) {}

    FuncNode(const FuncNode& other)
        : m_atoms(other.m_atoms), m_atom_index(other.m_atom_index)
    {
        switch (Arity()) {
            case 0:
                m_arg1 = nullptr;
                m_arg2 = nullptr;
                break;
            case 1:
                m_arg1 = std::make_unique<FuncNode>(*other.m_arg1);
                m_arg2 = nullptr;
                break;
            case 2:
                m_arg1 = std::make_unique<FuncNode>(*other.m_arg1);
                m_arg2 = std::make_unique<FuncNode>(*other.m_arg2);
                break;
            default:
                break;
        }
    }

    FuncNode(FuncNode&& other) noexcept
        : m_atoms(other.m_atoms), m_atom_index(other.m_atom_index)
    {
        switch (Arity()) {
            case 0:
                m_arg1 = nullptr;
                m_arg2 = nullptr;
                break;
            case 1:
                m_arg1 = std::move(other.m_arg1);
                m_arg2 = nullptr;
                break;
            case 2:
                m_arg1 = std::move(other.m_arg1);
                m_arg2 = std::move(other.m_arg2);
                break;
            default:
                break;
        }
    }

    FuncNode& operator=(const FuncNode& other) noexcept
    {
        m_atoms = other.m_atoms;
        m_atom_index = other.m_atom_index;
        switch (Arity()) {
            case 0:
                m_arg1 = nullptr;
                m_arg2 = nullptr;
                break;
            case 1:
                m_arg1 = std::make_unique<FuncNode>(*other.m_arg1);
                m_arg2 = nullptr;
                break;
            case 2:
                m_arg1 = std::make_unique<FuncNode>(*other.m_arg1);
                m_arg2 = std::make_unique<FuncNode>(*other.m_arg2);
                break;
            default:
                break;
        }
        return *this;
    }

    bool operator==(
        const FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other) const
    {
        if (m_atoms != other.m_atoms)
            return false;
        if (m_atom_index != other.m_atom_index)
            return false;

        if (m_arg1 == nullptr) {
            if (other.m_arg1 != nullptr)
                return false;
        }
        else {
            if (other.m_arg1 == nullptr)
                return false;
            if (*m_arg1 != *other.m_arg1)
                return false;
        }

        if (m_arg2 == nullptr) {
            if (other.m_arg2 != nullptr)
                return false;
        }
        else {
            if (other.m_arg2 == nullptr)
                return false;
            if (*m_arg2 != *other.m_arg2)
                return false;
        }

        return true;
    }

    std::size_t Arity() const { return m_atom_index.arity; }

    std::size_t FunctionsCount() const
    {
        switch (Arity()) {
            case 0:
                return 0;
            case 1:
                return (m_arg1->FunctionsCount() + 1);
            case 2:
                return (m_arg1->FunctionsCount() + m_arg2->FunctionsCount() +
                        1);
            default:
                return 0;
        }
        return 0;
    }

    std::size_t CurrentMaxLevel() const
    {
        switch (Arity()) {
            case 0:
                return 0;
            case 1:
                return (m_arg1->CurrentMaxLevel() + 1);
            case 2:
                return (std::max(m_arg1->CurrentMaxLevel(),
                                 m_arg2->CurrentMaxLevel()) +
                        1);
            default:
                return 0;
        }
        return 0;
    }

    std::size_t CurrentMinLevel() const
    {
        switch (Arity()) {
            case 0:
                return 0;
            case 1:
                return (m_arg1->CurrentMinLevel() + 1);
            case 2:
                return (std::min(m_arg1->CurrentMinLevel(),
                                 m_arg2->CurrentMinLevel()) +
                        1);
            default:
                return 0;
        }
        return 0;
    }

    SerialNumber_t MaxSerialNumber(std::size_t level) const
    {
        if (level == 0)
            return m_atoms->arg0.size();
        const auto max_prev = MaxSerialNumber(level - 1);
        const auto m = max_prev * max_prev * m_atoms->arg2.size() +
                       max_prev * m_atoms->arg1.size() + max_prev;
        return m;
    }

    SerialNumber_t SerialNumber() const
    {
        if (Arity() == 0)
            return m_atom_index.num;
        const auto level = CurrentMaxLevel();
        const auto max_prev = MaxSerialNumber(level - 1);
        std::size_t sn = max_prev;
        if (Arity() == 1) {
            sn += max_prev * m_atom_index.num;
            auto sn1 = m_arg1->SerialNumber();
            if (m_arg1->CurrentMaxLevel() > 0)
                sn1 -= m_atoms->arg0.size();
            sn += sn1;
        }
        else if (Arity() == 2) {
            sn += max_prev * m_atoms->arg1.size();
            sn += max_prev * max_prev * m_atom_index.num;
            auto sn1 = m_arg1->SerialNumber();
            auto sn2 = m_arg2->SerialNumber();
            sn += max_prev * sn2 + sn1;
        }
        return sn;
    }

    void ClearCalculated() { m_values.clear(); }

    const FuncValues_t& Calculate(bool recalculate = false)
    {
        if (m_values.empty() or recalculate) {
            switch (Arity()) {
                case 0:
                    m_values = m_atoms->arg0[m_atom_index.num]->Calculate();
                    break;
                case 1:
                    m_values = m_atoms->arg1[m_atom_index.num]->Calculate(
                        m_arg1->Calculate());
                    break;
                case 2:
                    m_values = m_atoms->arg2[m_atom_index.num]->Calculate(
                        m_arg1->Calculate(), m_arg2->Calculate());
                    break;
                default:
                    break;
            }
        }
        return m_values;
    }

    bool Constant()
    {
        switch (Arity()) {
            case 0:
                return m_atoms->arg0[m_atom_index.num]->Constant();
            case 1:
                return m_arg1->Constant();
            case 2:
                return (m_arg1->Constant() and m_arg2->Constant());
            default:
                break;
        }
        return true;
    }

    std::string Repr(std::string_view append = "") const
    {
        switch (Arity()) {
            case 0:
                return std::format(
                    "{}{}", m_atoms->arg0[m_atom_index.num]->Str(), append);
            case 1:
                return std::format("{}({}){}",
                                   m_atoms->arg1[m_atom_index.num]->Str(),
                                   m_arg1->Repr(), append);
            case 2:
                return std::format("{}({};{}){}",
                                   m_atoms->arg2[m_atom_index.num]->Str(),
                                   m_arg1->Repr(), m_arg2->Repr(), append);
            default:
                assert(false);
        }
        return {};
    }

    json ToJSON() const
    {
        json j;
        j["arity"] = m_atom_index.arity;
        j["num"] = m_atom_index.num;
        j["name"] = m_atoms->Get(m_atom_index.arity, m_atom_index.num)->Str();
        if (Arity() > 0)
            j["arg1"] = m_arg1->ToJSON();
        if (Arity() > 1)
            j["arg2"] = m_arg2->ToJSON();
        return j;
    }

    bool FromJSON(const json& j)
    {
        m_atom_index = AtomIndex{};
        m_arg1 = nullptr;
        m_arg2 = nullptr;

        const auto j_arity = j.find("arity");
        if (j_arity == j.end())
            return false;
        if (not j_arity->is_number_unsigned())
            return false;
        m_atom_index.arity = j_arity->get<std::size_t>();

        const auto j_num = j.find("num");
        if (j_num == j.end())
            return false;
        if (not j_num->is_number_unsigned())
            return false;
        m_atom_index.num = j_num->get<std::size_t>();

        if (Arity() > 0) {
            m_arg1 = std::make_unique<FuncNode>(m_atoms);
            const auto j_arg1 = j.find("arg1");
            if (j_arg1 == j.end())
                return false;
            if (not j_arg1->is_object())
                return false;
            if (not m_arg1->FromJSON(*j_arg1))
                return false;
        }

        if (Arity() > 1) {
            m_arg2 = std::make_unique<FuncNode>(m_atoms);
            const auto j_arg2 = j.find("arg2");
            if (j_arg2 == j.end())
                return false;
            if (not j_arg2->is_object())
                return false;
            if (not m_arg2->FromJSON(*j_arg2))
                return false;
        }

        return true;
    }

    bool InitDepth(const std::size_t max_depth,
                   const std::size_t current_depth = 0)
    {
        assert(current_depth <= max_depth);
        m_arg2 = nullptr;
        if (current_depth == max_depth) {
            m_arg1 = nullptr;
            m_atom_index.arity = 0;
            m_atom_index.num = 0;
        }
        else {
            m_arg1 = std::make_unique<FuncNode>(m_atoms);
            m_arg1->InitDepth(max_depth, current_depth + 1);
            m_atom_index.arity = 1;
            m_atom_index.num = 0;
        }
        return true;
    }

    bool Iterate(const std::size_t max_depth,
                 const std::size_t current_depth = 0)
    {
        bool keep_iterate = true;
        while (keep_iterate) {
            if (not IterateRaw(max_depth, current_depth))
                return false;
            keep_iterate = ((Arity() != 0) and SKIP_CONSTANT and Constant());
        }
        ClearCalculated();
        return true;
    }

    bool IterateArity0(const std::size_t max_depth,
                       const std::size_t next_depth)
    {
        if (LastArityFunc()) {
            if (next_depth > max_depth)
                return false;
            NextArity1();
        }
        else {
            ++m_atom_index.num;
        }
        return true;
    }

    bool IterateArity1(const std::size_t max_depth,
                       const std::size_t next_depth)
    {
        bool arg1_iterated = m_arg1->Iterate(max_depth, next_depth);

        if (SKIP_CONSTANT) {
            if (arg1_iterated and (m_arg1->Arity() == 0) and
                (m_arg1->Constant())) {
                arg1_iterated = false;
            }
        }

        if (not arg1_iterated) {
            if (LastArityFunc()) {
                NextArity2();
                m_arg2->InitDepth(max_depth, next_depth);
            }
            else {
                NextArity1();
                m_arg1->InitDepth(max_depth, next_depth);
            }
        }
        return true;
    }

    bool IterateArity2(const std::size_t max_depth,
                       const std::size_t next_depth)
    {
        bool arg1_iterated = m_arg1->Iterate(max_depth, next_depth);

        if (SKIP_CONSTANT) {
            if (arg1_iterated and (m_arg1->Arity() == 0) and
                (m_arg1->Constant()) and (m_arg2->Arity() == 0) and
                (m_arg2->Constant())) {
                arg1_iterated = false;
            }
        }

        if (SKIP_SYMMETRIC) {
            if (arg1_iterated and
                m_atoms->arg2[m_atom_index.num]->Commutative()) {
                if (m_atoms->arg2[m_atom_index.num]->Idempotent()) {
                    if (m_arg1->SerialNumber() >= m_arg2->SerialNumber()) {
                        arg1_iterated = false;
                    }
                }
                else {
                    if (m_arg1->SerialNumber() > m_arg2->SerialNumber()) {
                        arg1_iterated = false;
                    }
                }
            }
        }

        if (not arg1_iterated) {
            if (not m_arg2->Iterate(max_depth, next_depth)) {
                if (LastArityFunc()) {
                    return false;
                }
                else {
                    NextArity2();
                    m_arg2->InitDepth(max_depth, next_depth);
                }
            }
            else {
                m_arg1 = std::make_unique<FuncNode>(m_atoms);
            }
        }
        return true;
    }

    bool IterateRaw(const std::size_t max_depth,
                    const std::size_t current_depth)
    {
        bool result = false;
        const auto next_depth = current_depth + 1;
        const auto current_max_depth = current_depth + CurrentMaxLevel();
        if (Arity() == 0) {
            result = IterateArity0(current_max_depth, next_depth);
        }
        else if (Arity() == 1) {
            result = IterateArity1(current_max_depth, next_depth);
        }
        else if (Arity() == 2) {
            result = IterateArity2(current_max_depth, next_depth);
        }

        if (not result) {
            if (current_max_depth < max_depth) {
                InitDepth(current_max_depth + 1, current_depth);
                result = true;
            }
        }

        return result;
    }

   private:
    AtomFuncs_t* m_atoms = nullptr;
    AtomIndex m_atom_index;

    std::unique_ptr<FuncNode> m_arg1 = nullptr;
    std::unique_ptr<FuncNode> m_arg2 = nullptr;

    FuncValues_t m_values;

    bool LastArityFunc() const
    {
        switch (Arity()) {
            case 0:
                return (m_atom_index.num + 1 >= m_atoms->arg0.size());
            case 1:
                return (m_atom_index.num + 1 >= m_atoms->arg1.size());
            case 2:
                return (m_atom_index.num + 1 >= m_atoms->arg2.size());
            default:
                assert(false);
        }
        return false;
    }

    void NextArity1()
    {
        if (Arity() != 1) {
            m_atom_index.arity = 1;
            m_atom_index.num = 0;
        }
        else {
            ++m_atom_index.num;
        }
        m_arg1 = std::make_unique<FuncNode>(m_atoms);
        m_arg2 = nullptr;
    }

    void NextArity2()
    {
        if (Arity() != 2) {
            m_atom_index.arity = 2;
            m_atom_index.num = 0;
        }
        else {
            ++m_atom_index.num;
        }
        m_arg1 = std::make_unique<FuncNode>(m_atoms);
        m_arg2 = std::make_unique<FuncNode>(m_atoms);
    }
};
}  // namespace fw