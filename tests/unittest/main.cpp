#include <gtest/gtest.h>
#include <print>
#include <set>

#include <func_node.h>

#include "atom_samples.h"
#include "search_task.h"

class TestTarget : public Target<uint16_t>
{
   public:
    ~TestTarget() final = default;

    TestTarget()
    {
        m_values.clear();
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            m_values.push_back(i);
        }
    }

    Distance Compare(const FuncValues_t& values) const final
    {
        Distance dist{};
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            if (values[i] != Values()[i]) {
                ++dist;
            }
        }
        return dist;
    }

    RangeSet<std::size_t> MatchPositions(const FuncValues_t& values) const final
    {
        RangeSet<std::size_t> rset;
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            if (values[i] == Values()[i]) {
                rset.Add(i);
            }
        }
        return rset;
    }

    FuncValues_t Values() const final { return m_values; }

   private:
    FuncValues_t m_values;
};

auto MakeAtoms() -> AtomFuncs<uint16_t>
{
    constexpr uint16_t MAX_CONSTANTS = 3;
    AtomFuncs<uint16_t> atoms;
    atoms.arg0.push_back(new AF_ARG_X{});
    for (uint16_t i = 1; i <= MAX_CONSTANTS; ++i) {
        atoms.arg0.push_back(new AF_CONST{i});
    }
    atoms.arg1.push_back(new AF_NOT{});
    atoms.arg1.push_back(new AF_BITCOUNT{});
    atoms.arg2.push_back(new AF_SUM{});
    atoms.arg2.push_back(new AF_AND{});
    atoms.arg2.push_back(new AF_OR{});
    return atoms;
}

TEST(FUNC_ITERATOR, SERIAL_NUMBER)
{
    AtomFuncs<uint16_t> atoms = MakeAtoms();
    FuncNode<uint16_t> fnc{&atoms};
    auto snum = fnc.SerialNumber();
    ASSERT_EQ(snum, 0);
    std::size_t snum_old = snum;
    std::size_t snum_etalon = 0;
    while (fnc.Iterate(2)) {
        ++snum_etalon;
        snum = fnc.SerialNumber();
        //if (snum_etalon == 839)
        //printf("%s %zu %zu\n", fnc.Repr().c_str(), snum, snum_etalon);
        ASSERT_GT(snum, snum_old);
        snum_old = snum;
    }
}

TEST(FUNC_ITERATOR, SKIP_SYMMETRIC)
{
    AtomFuncs<uint16_t> atoms = MakeAtoms();
    FuncNode<uint16_t, false, true> fnc{&atoms};
    ASSERT_EQ(fnc.Repr(), "X");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "1");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "2");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "3");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "NOT(X)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "NOT(1)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "NOT(2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "NOT(3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "BITCOUNT(X)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "BITCOUNT(1)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "BITCOUNT(2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "BITCOUNT(3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(X;X)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(X;1)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(1;1)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(X;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(1;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(2;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(X;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(1;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(2;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "SUM(3;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(X;X)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(X;1)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(X;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(1;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(X;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(1;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "AND(2;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(X;X)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(X;1)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(X;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(1;2)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(X;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(1;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "OR(2;3)");
    ASSERT_TRUE(fnc.Iterate(2));
    ASSERT_EQ(fnc.Repr(), "NOT(NOT(X))");
}

TEST(SEARCH_TASK, JSON)
{
    constexpr std::size_t MAX_BEST = 5;
    constexpr std::size_t MAX_ITERATIONS = 100;

    AtomFuncs<uint16_t> atoms = MakeAtoms();
    Settings settings;
    settings.max_best = MAX_BEST;
    settings.max_depth = 2;
    TestTarget target{};
    ASSERT_FALSE(target.Values().empty());
    SearchTask<uint16_t, true, true> task{settings, &atoms, &target};
    for (std::size_t i = 0; i < MAX_ITERATIONS; ++i) {
        ASSERT_TRUE(task.SearchIterate());
        const auto json_str = task.ToJSON().dump();
        SearchTask<uint16_t, true, true> new_task{settings, &atoms, &target};
        ASSERT_TRUE(new_task.FromJSON(json_str));
        std::println("{}", new_task.Status());
        ASSERT_EQ(task, new_task);
    }
    ASSERT_TRUE(true);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}