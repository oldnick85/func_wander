#include <iostream>

#include "atom_samples.h"
#include "target_sample.h"
#include "src/func_iterator.h"

/*
void Pool()
{
    std::vector<AtomFunc> atoms;
    atoms.push_back(AtomFunc{new AF_ARG_X{}});
    atoms.push_back(AtomFunc{new AF_CONST<1>{}});
    atoms.push_back(AtomFunc{new AF_CONST<2>{}});
    atoms.push_back(AtomFunc{new AF_CONST<3>{}});
    atoms.push_back(AtomFunc{new AF_CONST<4>{}});
    atoms.push_back(AtomFunc{new AF_CONST<5>{}});
    atoms.push_back(AtomFunc{new AF_CONST<6>{}});
    atoms.push_back(AtomFunc{new AF_CONST<7>{}});
    atoms.push_back(AtomFunc{new AF_CONST<8>{}});
    atoms.push_back(AtomFunc{new AF_SUM{}});
    atoms.push_back(AtomFunc{new AF_AND{}});
    atoms.push_back(AtomFunc{new AF_OR{}});
    atoms.push_back(AtomFunc{new AF_SHR{}});
    atoms.push_back(AtomFunc{new AF_SHL{}});
    MyTarget target;
    FuncPool pool{atoms, &target};
    std::cout << target.StrFull() << std::endl;
    std::cout << "POOL: " << pool.StrBest() << std::endl;
    pool.Step();
    std::cout << "POOL: " << pool.StrBest() << std::endl;
    pool.Step();
    std::cout << "POOL: " << pool.StrBest() << std::endl;
    pool.Step();
    std::cout << "POOL: " << pool.StrBest() << std::endl;
    pool.Step();
    std::cout << "POOL: " << pool.StrBest() << std::endl;
}
*/

void Search()
{
    AtomFuncs atoms;
    atoms.arg0.push_back(new AF_ARG_X{});
    atoms.arg0.push_back(new AF_CONST<1>{});
    atoms.arg0.push_back(new AF_CONST<2>{});
    atoms.arg0.push_back(new AF_CONST<3>{});
    atoms.arg0.push_back(new AF_CONST<4>{});
    atoms.arg0.push_back(new AF_CONST<5>{});
    atoms.arg0.push_back(new AF_CONST<6>{});
    atoms.arg0.push_back(new AF_CONST<7>{});
    atoms.arg0.push_back(new AF_CONST<8>{});
    atoms.arg1.push_back(new AF_NOT{});
    atoms.arg1.push_back(new AF_BITCOUNT{});
    atoms.arg2.push_back(new AF_SUM{});
    atoms.arg2.push_back(new AF_AND{});
    atoms.arg2.push_back(new AF_OR{});
    atoms.arg2.push_back(new AF_SHR{});
    atoms.arg2.push_back(new AF_SHL{});
    MyTarget target;
    FuncIterator<true> it{&atoms, {}};
    std::cout << "IT: " << it.Repr() << std::endl;
    std::size_t c = 0;
    while (it.Iterate(0, 1, 0)) {
        const auto d = target.Compare(it.Calculate());
        //if (d <= 224)
        printf("IT: sn=%zu | %s lvl=%zu maxlvl=%zu d=%zu\n", it.SerialNumber(),
               it.Repr().c_str(), it.CurrentLevel(),
               it.MaxSerialNumber(it.CurrentLevel()), d);
        ++c;
        if ((c % 1000000) == 0)
            std::cout << c << std::endl;
    }
}

int main()
{
    Search();
    return 0;
}