#include "fsc.h"

#include <format>

NodeFunc::NodeFunc(AtomFunc self_func, NodeFunc* arg1, NodeFunc* arg2)
    : m_self_func(self_func), m_arg1(arg1), m_arg2(arg2)
{
    switch (m_self_func.index()) {
        case 0:
            assert(arg1 == nullptr);
            assert(arg2 == nullptr);
            break;
        case 1:
            assert(arg1 != nullptr);
            assert(arg2 == nullptr);
            break;
        case 2:
            assert(arg1 != nullptr);
            assert(arg2 != nullptr);
            break;
        default:
            break;
    }
}

FuncValues NodeFunc::Calculate() const
{
    FuncValues result;
    switch (m_self_func.index()) {
        case 0:
            result = std::get<0>(m_self_func)->Calculate();
            break;
        case 1:
            result = std::get<1>(m_self_func)->Calculate(m_arg1->Calculate());
            break;
        case 2:
            result = std::get<2>(m_self_func)
                         ->Calculate(m_arg1->Calculate(), m_arg2->Calculate());
            break;
        default:
            assert(false);
    }
    return result;
}

std::string NodeFunc::Name() const
{
    switch (m_self_func.index()) {
        case 0:
            assert(std::get<0>(m_self_func) != nullptr);
            return std::format("{}[{}]", std::get<0>(m_self_func)->Str(),
                               reinterpret_cast<const void*>(this));
        case 1:
            assert(std::get<1>(m_self_func) != nullptr);
            return std::format("{}[{}]", std::get<1>(m_self_func)->Str(),
                               reinterpret_cast<const void*>(this));
        case 2:
            assert(std::get<2>(m_self_func) != nullptr);
            return std::format("{}[{}]", std::get<2>(m_self_func)->Str(),
                               reinterpret_cast<const void*>(this));
        default:
            assert(false);
    }
    return {};
}

std::string NodeFunc::Str() const
{
    switch (m_self_func.index()) {
        case 0:
            assert(std::get<0>(m_self_func) != nullptr);
            return std::format("NODE {}", Name());
        case 1:
            assert(std::get<1>(m_self_func) != nullptr);
            return std::format("NODE {} ({})", Name(), m_arg1->Str());
        case 2:
            assert(std::get<2>(m_self_func) != nullptr);
            return std::format("NODE {} ({} ; {})", Name(), m_arg1->Name(),
                               m_arg2->Name());
        default:
            assert(false);
    }
    return {};
}

std::string NodeFunc::Repr() const
{
    switch (m_self_func.index()) {
        case 0:
            assert(std::get<0>(m_self_func) != nullptr);
            return std::format("{}", std::get<0>(m_self_func)->Str());
        case 1:
            assert(std::get<1>(m_self_func) != nullptr);
            return std::format("{}({})", std::get<1>(m_self_func)->Str(),
                               m_arg1->Repr());
        case 2:
            assert(std::get<2>(m_self_func) != nullptr);
            return std::format("{}({} ; {})", std::get<2>(m_self_func)->Str(),
                               m_arg1->Repr(), m_arg2->Repr());
        default:
            assert(false);
    }
    return {};
}

std::string NodeFunc::StrFull() const
{
    std::string res = Str() + "\n" + Repr() + "\n";
    const auto result = Calculate();
    for (auto& v : result)
        res += std::string("; ") + std::to_string(v);
    return res;
}