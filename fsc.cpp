#include "fsc.h"

#include <format>

FuncPool::FuncPool(std::vector<AtomFunc> atoms, Target* target)
    : m_atoms(atoms), m_target(target)
{
}

bool CheckBest(NodeFunc* new_func)
{
    bool res = false;

    if (m_best.empty()) {
        m_best.push_back(new_func);
        return true;
    }

    const auto new_dist = m_target->Compare(new_func->Calculate());
    auto it = m_best.begin();
    while (it != m_best.end()) {
        const auto dist = m_target->Compare((*it)->Calculate());
        if (new_dist < dist) {
            m_best.insert(it, new_func);
            res = true;
            break;
        }
        ++it;
    }

    if (m_best.size() > 5)
        m_best.resize(5);
        
    return res;
}

void FuncPool::Step()
{
    if (m_funcs.empty()) {
        m_funcs.push_back({});
        auto& new_funcs = m_funcs.back();
        for (auto& atom_func : m_atoms) {
            if (atom_func.index() == 0) {
                auto* new_func = new NodeFunc(atom_func, nullptr, nullptr);
                new_funcs.push_back(new_func);
                if (CheckBest(new_func))
                    printf("new: %zu\n%s\n", new_funcs.size(),
                           StrBest().c_str());
            }
        }
        m_level = 0;
        return;
    }

    std::vector<NodeFunc*> new_funcs;

    for (auto& atom_func : m_atoms) {
        if (atom_func.index() == 2) {
            AtomFunc2* atom_func2 = std::get<2>(atom_func);
            StepFunc2(atom_func2, new_funcs);
        }
        if (atom_func.index() == 1) {
            AtomFunc1* atom_func1 = std::get<1>(atom_func);
            StepFunc1(atom_func1, new_funcs);
        }
    }

    ++m_level;
    m_funcs.push_back(new_funcs);
}

void FuncPool::StepFunc2(AtomFunc2* atom_func2,
                         std::vector<NodeFunc*>& new_funcs)
{
    std::size_t i_lvl = m_level;
    auto& i_funcs = m_funcs[i_lvl];
    for (std::size_t i_fnc = 0; i_fnc < i_funcs.size(); ++i_fnc) {
        auto& i_func = i_funcs[i_fnc];
        for (std::size_t j_lvl = 0; j_lvl < m_funcs.size(); ++j_lvl) {
            auto& j_funcs = m_funcs[j_lvl];
            for (std::size_t j_fnc = 0; j_fnc < j_funcs.size(); ++j_fnc) {
                auto& j_func = j_funcs[j_fnc];
                if (IsConstant(i_func) and IsConstant(j_func))
                    continue;
                if ((atom_func2->Commutative()) and (j_lvl == i_lvl) and
                    (j_fnc < i_fnc))
                    continue;
                auto* new_func = new NodeFunc{atom_func2, i_func, j_func};
                const auto result = new_func->Calculate();
                if (IsConstant(result)) {
                    delete new_func;
                    continue;
                }
                new_funcs.push_back(new_func);
                if (CheckBest(new_func))
                    printf("\n\nnew: %zu\n%s\n", new_funcs.size(),
                           StrBest().c_str());
            }
        }
    }
}

bool FuncPool::IsConstant(NodeFunc* func) const
{
    return ((func->Func().index() == 0) and
            std::get<0>(func->Func())->Constant());
}

bool FuncPool::IsConstant(const FuncValues& result) const
{
    for (auto v : result) {
        if (v != result.front())
            return false;
    }
    return true;
}

void FuncPool::StepFunc1(AtomFunc1* atom_func1,
                         std::vector<NodeFunc*>& new_funcs)
{
    std::size_t i_lvl = m_level;
    auto& i_funcs = m_funcs[i_lvl];
    for (std::size_t i_fnc = 0; i_fnc < i_funcs.size(); ++i_fnc) {
        auto& i_func = i_funcs[i_fnc];
        if (IsConstant(i_func))
            continue;
        auto new_func = new NodeFunc{atom_func1, i_func, nullptr};
        const auto result = new_func->Calculate();
        if (IsConstant(result)) {
            delete new_func;
            continue;
        }
        new_funcs.push_back(new_func);
        if (CheckBest(new_func))
            printf("new: %zu\n%s\n", new_funcs.size(), StrBest().c_str());
    }
}

std::string FuncPool::Str() const
{
    std::string res = "count=";
    for (auto& funcs_level : m_funcs)
        res += std::to_string(funcs_level.size()) + "; ";
    return res;
}

std::string FuncPool::StrFull() const
{
    std::string res = Str() + "\n";
    for (auto& funcs_level : m_funcs)
        for (auto& func_node : funcs_level)
            res += std::string("  ") + func_node->StrFull() + "\n";
    return res;
}

std::string FuncPool::StrBest() const
{
    std::string res = Str() + "\n";
    for (auto& func : m_best) {
        const auto dist = m_target->Compare(func->Calculate());
        res += std::string("    ") + std::to_string(dist) + " : " +
               func->StrFull() + "\n";
    }
    return res;
}