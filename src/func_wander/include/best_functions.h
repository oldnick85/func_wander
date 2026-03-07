#pragma once

#include <list>

#include "comparison.h"
#include "func_node.h"
#include "target.h"

namespace fw
{

template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class BestPool
{
   public:
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
        if (m_best.empty()) {
            m_best.push_back(fnc);
            return;
        }

        const auto fnc_calc = fnc.Calculate();
        const auto fnc_ranges = target->MatchPositions(fnc_calc);
        const auto new_dist = CalcDist(fnc, target);
        if (m_best.size() >= max_best) {
            if (new_dist > m_suit_threshold) {
                return;
            }
        }

        auto best_it = m_best.begin();
        while (best_it != m_best.end()) {
            const auto dist = CalcDist(*best_it, target);
            if (new_dist < dist) {
                // Check for uniqueness to avoid duplicates
                bool unique_values = true;
                for (auto& b : m_best) {
                    const auto b_calc = b.Calculate();
                    const auto b_ranges = target->MatchPositions(b_calc);
                    if (b_calc == fnc_calc) {
                        unique_values = false;
                        break;
                    }
                    if (b_ranges == fnc_ranges) {
                        unique_values = false;
                        break;
                    }
                }
                if (not unique_values) {
                    break;
                }
                m_best.insert(best_it, fnc);
                break;
            }
            ++best_it;
        }

        // Maintain maximum list size
        while (m_best.size() > max_best) {
            m_best.pop_back();
        }

        // Update threshold to worst distance in current best list
        m_suit_threshold = CalcDist(m_best.back(), target);
    }

    void CheckBest(BestPool<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>& other, Target<FuncValue_t>* target,
                   std::size_t max_best)
    {
        for (auto& func : other.Functions()) {
            CheckBest(func, target, max_best);
        }
    }

    const std::list<FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>>& Functions() const { return m_best; }
    std::list<FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>>& Functions() { return m_best; }
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
    std::list<FuncNode<FuncValue_t, SKIP_CONSTANT, SKIP_SYMMETRIC>>
        m_best;                           ///< 🏆 Best functions found (maintained in order)
    SuitabilityMetrics m_suit_threshold;  ///< 📊 Worst distance currently in best list
};

}  // namespace fw