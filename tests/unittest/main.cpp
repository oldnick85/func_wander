#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <print>
#include <vector>

#include <atom_samples.h>
#include <common.h>
#include <func_node.h>
#include <search_task.h>
#include <target.h>

using fw::AtomFuncs;
using fw::Distance;
using fw::FuncNode;
using fw::RangeSet;
using fw::SearchTask;
using fw::Settings;
using fw::Target;

class TestTarget : public Target<uint16_t>
{
   public:
    ~TestTarget() override = default;

    TestTarget()
    {
        m_values.clear();
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            m_values.push_back(i);
        }
    }

    [[nodiscard]] Distance Compare(const FuncValues_t& values) const override
    {
        Distance dist{};
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            if (values[i] != Values()[i]) {
                ++dist;
            }
        }
        return dist;
    }

    [[nodiscard]] RangeSet<std::size_t> MatchPositions(const FuncValues_t& values) const override
    {
        RangeSet<std::size_t> rset;
        for (std::size_t i = 0; i < VALUES_RANGE; ++i) {
            if (values[i] == Values()[i]) {
                rset.Add(i);
            }
        }
        return rset;
    }

    [[nodiscard]] FuncValues_t Values() const override { return m_values; }

   private:
    FuncValues_t m_values;
};

namespace
{

std::unique_ptr<AF_ARG_X> af_x;
std::vector<std::unique_ptr<AF_CONST>> af_c;
std::unique_ptr<AF_NOT> af_not;
std::unique_ptr<AF_BITCOUNT> af_bc;
std::unique_ptr<AF_SUM> af_sum;
std::unique_ptr<AF_AND> af_and;
std::unique_ptr<AF_OR> af_or;

auto MakeAtoms() -> AtomFuncs<uint16_t>
{
    constexpr uint16_t MAX_CONSTANTS = 3;
    AtomFuncs<uint16_t> atoms;
    af_x = std::make_unique<AF_ARG_X>();
    atoms.arg0.push_back(af_x.get());
    for (uint16_t i = 1; i <= MAX_CONSTANTS; ++i) {
        af_c.emplace_back(std::make_unique<AF_CONST>(i));
        atoms.arg0.push_back(af_c.back().get());
    }
    af_not = std::make_unique<AF_NOT>();
    atoms.arg1.push_back(af_not.get());
    af_bc = std::make_unique<AF_BITCOUNT>();
    atoms.arg1.push_back(af_bc.get());
    af_sum = std::make_unique<AF_SUM>();
    atoms.arg2.push_back(af_sum.get());
    af_and = std::make_unique<AF_AND>();
    atoms.arg2.push_back(af_and.get());
    af_or = std::make_unique<AF_OR>();
    atoms.arg2.push_back(af_or.get());
    return atoms;
}

}  // namespace

// NOLINTBEGIN(readability-function-cognitive-complexity, readability-function-size)
TEST(FuncIterator, SerialNumber)
{
    AtomFuncs<uint16_t> atoms = MakeAtoms();
    FuncNode<uint16_t> fnc{&atoms};
    auto snum = fnc.SerialNumber();
    ASSERT_EQ(snum, 0);
    std::size_t snum_old = snum;
    //std::size_t snum_etalon = 0;
    while (fnc.Iterate(2)) {
        //++snum_etalon;
        snum = fnc.SerialNumber();
        //if (snum_etalon == 839)
        //printf("%s %zu %zu\n", fnc.Repr().c_str(), snum, snum_etalon);
        ASSERT_GT(snum, snum_old);
        snum_old = snum;
    }
}

TEST(FuncIterator, SkipSymmetric)
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

TEST(SearchTask, JSON)
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
// NOLINTEND(readability-function-cognitive-complexity, readability-function-size)

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}