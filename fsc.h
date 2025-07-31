#pragma once

#include "node.h"
#include "src/target.h"

class FuncPool
{
   public:
    FuncPool(std::vector<AtomFunc> atoms, Target* target);

    void Step();

    std::string Str() const;
    std::string StrFull() const;
    std::string StrBest() const;

   private:
    bool IsConstant(NodeFunc* func) const;
    bool IsConstant(const FuncValues& result) const;

    void StepFunc1(AtomFunc1* atom_func1, std::vector<NodeFunc*>& new_funcs);
    void StepFunc2(AtomFunc2* atom_func2, std::vector<NodeFunc*>& new_funcs);

    bool CheckBest(NodeFunc* new_func);

    std::vector<AtomFunc> m_atoms;
    Target* m_target = nullptr;
    std::vector<std::vector<NodeFunc*>> m_funcs;
    std::vector<NodeFunc*> m_best;
    int m_level = -1;
};