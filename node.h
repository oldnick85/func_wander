#pragma once

#include "src/atom.h"

class NodeFunc
{
   public:
    NodeFunc(AtomFunc self_func, NodeFunc* arg1, NodeFunc* arg2);

    FuncValues Calculate() const;

    AtomFunc& Func() { return m_self_func; }

    std::string Name() const;
    std::string Str() const;
    std::string Repr() const;
    std::string StrFull() const;

   private:
    AtomFunc m_self_func;

    NodeFunc* m_arg1 = nullptr;
    NodeFunc* m_arg2 = nullptr;
};