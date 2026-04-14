#pragma once

#include <list>
#include <ranges>

#include "comparison.h"
#include "func_node.h"
#include "target.h"

namespace fw
{

template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class BestFunc
{
   public:
    using FuncValues_t = std::vector<FuncValue_t>;

    BestFunc(FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& func_, const FuncValues_t& calc_,
             const SuitabilityMetrics& suit_, const RangeSet<std::size_t>& ranges_)
        : func(func_), calc(calc_), suit(suit_), ranges(ranges_)
    {
    }

    FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC> func;
    FuncValues_t calc;
    SuitabilityMetrics suit;
    RangeSet<std::size_t> ranges;
};

template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class BestPool
{
   public:
    using BestFunc_t = BestFunc<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>;

    void Reset()
    {
        m_best.clear();
        m_suit_threshold.Reset();
    }
    /**
     * @brief Evaluate and potentially add function to best list
     * @param fnc Candidate function tree
     * @param max_best Maximum size of best list
     * 
     * Algorithm:
     * 1. Calculate distance score
     * 2. Skip if worse than current threshold and list is full
     * 3. Check for uniqueness (exact values or matching positions)
     * 4. Insert in sorted position
     * 5. Trim list if exceeds max_best
     * 6. Update distance threshold
     */
    void CheckBest(FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& fnc, Target<FuncValue_t>* target,
                   std::size_t max_best)
    {
        const auto fnc_calc = fnc.Calculate();
        const auto ranges = target->MatchPositions(fnc_calc);

        if (ranges.Count() == 0) {
            return;
        }

        BestFunc_t fnc_kit{fnc, fnc_calc, CalcDist(fnc, target), ranges};

        if (m_best.empty()) {
            m_best.push_back(fnc_kit);
            return;
        }
        else {
            if (m_best.size() >= max_best) {
                if (fnc_kit.suit > m_suit_threshold) {
                    return;
                }
            }

            // Check for uniqueness to avoid duplicates
            if (not UniqueValues(fnc_kit, target)) {
                return;
            }

            auto best_it = m_best.begin();
            while (best_it != m_best.end()) {
                if (fnc_kit.suit < best_it->suit) {
                    break;
                }
                ++best_it;
            }
            m_best.insert(best_it, fnc_kit);

            // Maintain maximum list size
            while (m_best.size() > max_best) {
                m_best.pop_back();
            }
        }

        // Update threshold to worst distance in current best list
        m_suit_threshold = m_best.back().suit;
    }

    void CheckBest(BestPool<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other, Target<FuncValue_t>* target,
                   std::size_t max_best)
    {
        for (auto& func : other.Functions()) {
            CheckBest(func, target, max_best);
        }
    }

    bool UniqueValues(const BestFunc_t& fnc_kit, Target<FuncValue_t>* target)
    {
        for (auto& b : m_best) {
            const auto b_calc = b.func.Calculate();
            const auto b_ranges = target->MatchPositions(b_calc);
            if (b.calc == fnc_kit.calc) {
                return false;
            }
            if (b.ranges == fnc_kit.ranges) {
                return false;
            }
            if (b.func.SerialNumber() == fnc_kit.func.SerialNumber()) {
                std::println("!");
            }
            if (b.func.Repr() == fnc_kit.func.Repr()) {
                std::println("!");
            }
        }
        return true;
    }

    const std::list<BestFunc<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>>& FunctionsKit() const { return m_best; }

    std::list<FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>> Functions()
    {
        auto funcs = m_best | std::views::transform(&BestFunc<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>::func) |
                     std::ranges::to<std::list>();
        return funcs;
    }

    const SuitabilityMetrics& SuitThreshold() const { return m_suit_threshold; }
    void SetSuitThreshold(const SuitabilityMetrics& suit_threshold) { m_suit_threshold = suit_threshold; }

    bool operator==(const BestPool<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other) const
    {
        if (m_best != other.m_best) {
            return false;
        }
        if (m_suit_threshold != other.m_suit_threshold) {
            return false;
        }
        return true;
    }

   private:
    std::list<BestFunc_t> m_best;         ///< 🏆 Best functions found (maintained in order)
    SuitabilityMetrics m_suit_threshold;  ///< 📊 Worst distance currently in best list
};

}  // namespace fw