#pragma once

#include <format>
#include <memory>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "atom.h"

namespace fw
{

/// @addtogroup FunctionNodes
/// @{

/**
 * @struct AtomIndex
 * @brief Index to identify an atomic function in the function library
 * 
 * Used to uniquely identify a function by its arity and position
 * in the corresponding vector of functions.
 */
struct AtomIndex
{
    /// @brief Equality comparison operator
    bool operator==(const AtomIndex& other) const { return ((arity == other.arity) and (num == other.num)); }

    std::size_t arity = 0;  ///< Arity of function (0, 1, or 2)
    std::size_t num = 0;    ///< Index in the corresponding arity vector
};

/**
 * @class AtomFuncs
 * @brief Container for all available atomic functions
 * @tparam FuncValue_t Type of function values
 * 
 * Stores and manages all atomic functions organized by arity.
 * Functions are grouped into vectors based on their number of arguments.
 */
template <typename FuncValue_t>
class AtomFuncs
{
   public:
    /**
     * @brief Add a nullary function
     * @param func Pointer to nullary function
     * 
     * Constant functions are placed at the end of the list.
     */
    void Add(AtomFunc0<FuncValue_t>* func)
    {
        if (func->Constant()) {
            arg0.push_back(func);
        }
        else {
            arg0.insert(arg0.begin(), func);
        }
    }

    /// @brief Add a unary function
    void Add(AtomFunc1<FuncValue_t>* func) { arg1.push_back(func); }

    /// @brief Add a binary function
    void Add(AtomFunc2<FuncValue_t>* func) { arg2.push_back(func); }

    /**
     * @brief Get atomic function by arity and index
     * @param arity Arity of function (0, 1, or 2)
     * @param num Index in the arity vector
     * @return Pointer to atomic function, or nullptr if invalid
     */
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

    std::vector<AtomFunc0<FuncValue_t>*> arg0;  ///< Nullary functions (constants at the end)
    std::vector<AtomFunc1<FuncValue_t>*> arg1;  ///< Unary functions
    std::vector<AtomFunc2<FuncValue_t>*> arg2;  ///< Binary functions
};

/**
 * @class FuncNode
 * @brief Represents a node in a function expression tree
 * @tparam FuncValue_t Type of function values
 * @tparam SKIP_CONSTANT Whether to skip constant sub-expressions during iteration
 * @tparam SKIP_SYMMETRIC Whether to skip symmetric duplicates for commutative operations
 * 
 * This class implements a tree structure where each node is either:
 * - A leaf: nullary function (constant or variable)
 * - A unary node: function applied to one child
 * - A binary node: function applied to two children
 * 
 * The tree can be evaluated, serialized, and iterated over.
 */
template <typename FuncValue_t, bool SKIP_CONSTANT = false, bool SKIP_SYMMETRIC = false>
class FuncNode
{
   public:
    /// Vector type for function values
    using FuncValues_t = std::vector<FuncValue_t>;
    /// Type alias for atomic function container
    using AtomFuncs_t = AtomFuncs<FuncValue_t>;

    /**
     * @brief Construct a FuncNode with reference to atomic functions
     * @param atoms Pointer to container of atomic functions
     */
    explicit FuncNode(AtomFuncs_t* atoms) : m_atoms(atoms) {}

    /// @brief Copy constructor (deep copy)
    FuncNode(const FuncNode& other) : m_atoms(other.m_atoms), m_atom_index(other.m_atom_index)
    {
        // Deep copy children based on arity
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

    /// @brief Move constructor
    FuncNode(FuncNode&& other) noexcept : m_atoms(other.m_atoms), m_atom_index(other.m_atom_index)
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

    /// @brief Copy assignment operator
    FuncNode& operator=(const FuncNode& other) noexcept
    {
        if (this != &other) {
            m_atoms = other.m_atoms;
            m_atom_index = other.m_atom_index;

            // Reconstruct children based on arity
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
        return *this;
    }

    /**
     * @brief Equality comparison operator
     * @param other FuncNode to compare with
     * @return true if trees are structurally and functionally equal
     */
    bool operator==(const FuncNode& other) const
    {
        if (m_atoms != other.m_atoms) {
            return false;
        }
        if (m_atom_index != other.m_atom_index) {
            return false;
        }

        // Compare children recursively
        if (m_arg1 == nullptr) {
            if (other.m_arg1 != nullptr) {
                return false;
            }
        }
        else {
            if (other.m_arg1 == nullptr) {
                return false;
            }
            if (*m_arg1 != *other.m_arg1) {
                return false;
            }
        }

        if (m_arg2 == nullptr) {
            if (other.m_arg2 != nullptr) {
                return false;
            }
        }
        else {
            if (other.m_arg2 == nullptr) {
                return false;
            }
            if (*m_arg2 != *other.m_arg2) {
                return false;
            }
        }

        return true;
    }

    /// @brief Get arity of this node (0, 1, or 2)
    [[nodiscard]] std::size_t Arity() const { return m_atom_index.arity; }

    void UniqFunctionsSerialNumbers(std::unordered_set<SerialNumber_t, SerialNumberHash>& uniqs) const
    {
        switch (Arity()) {
            case 0:
                return;
            case 1:
                m_arg1->UniqFunctionsSerialNumbers(uniqs);
                break;
            case 2:
                m_arg1->UniqFunctionsSerialNumbers(uniqs);
                m_arg2->UniqFunctionsSerialNumbers(uniqs);
                break;
        }
        uniqs.insert(SerialNumber());
    }

    /// @brief Count total number of function nodes in the tree
    [[nodiscard]] std::size_t FunctionsCount() const
    {
        switch (Arity()) {
            case 0:
                return 0;
            case 1:
                return (m_arg1->FunctionsCount() + 1);
            case 2:
                return (m_arg1->FunctionsCount() + m_arg2->FunctionsCount() + 1);
            default:
                return 0;
        }
        return 0;
    }

    /// @brief Get maximum depth of the tree (height)
    [[nodiscard]] std::size_t CurrentMaxLevel() const
    {
        switch (Arity()) {
            case 0:
                return 0;
            case 1:
                return (m_arg1->CurrentMaxLevel() + 1);
            case 2:
                return (std::max(m_arg1->CurrentMaxLevel(), m_arg2->CurrentMaxLevel()) + 1);
            default:
                return 0;
        }
        return 0;
    }

    /// @brief Get minimum depth to any leaf node
    [[nodiscard]] std::size_t CurrentMinLevel() const
    {
        switch (Arity()) {
            case 0:
                return 0;
            case 1:
                return (m_arg1->CurrentMinLevel() + 1);
            case 2:
                return (std::min(m_arg1->CurrentMinLevel(), m_arg2->CurrentMinLevel()) + 1);
            default:
                return 0;
        }
        return 0;
    }

    /**
 * @brief Maximum number of distinct trees with depth up to given level.
 *
 * Recursively computes the total count of trees whose depth is not greater
 * than `level`. The count respects the canonical form for binary nodes:
 * the right subtree must have depth exactly one less than the whole tree,
 * while the left subtree can have any depth ≤ that of the whole tree.
 *
 * Let:
 * - \( A_0 = |\text{arg0}| \)  (number of leaf atoms)
 * - \( A_1 = |\text{arg1}| \)  (number of unary atoms)
 * - \( A_2 = |\text{arg2}| \)  (number of binary atoms)
 * - \( M(l) \) = number of trees with depth ≤ l, with \( M(-1) = 0 \).
 *
 * Then for \( l \ge 0 \):
 * \[
 * M(0) = A_0
 * \]
 * \[
 * M(l) = M(l-1) \;+\; \bigl(M(l-1)-M(l-2)\bigr) \cdot A_1
 *        \;+\; M(l-1) \cdot \bigl(M(l-1)-M(l-2)\bigr) \cdot A_2
 * \]
 * Explanation:
 * - Trees of depth ≤ l-1 are already counted in \( M(l-1) \).
 * - New trees of **exact** depth l are built from a unary or binary root
 *   whose subtrees reach depth l-1 exactly once.
 * - Exactly \( M(l-1)-M(l-2) \) trees have depth exactly l-1.
 * - Unary trees of depth l: choose any unary atom (\(A_1\) ways) and
 *   a subtree of depth exactly l-1 → \( (M(l-1)-M(l-2)) \cdot A_1 \).
 * - Binary trees of depth l: choose any binary atom (\(A_2\) ways);
 *   the left subtree can be any tree of depth ≤ l-1 (\(M(l-1)\) choices);
 *   the right subtree must have depth exactly l-1 (\(M(l-1)-M(l-2)\) choices).
 *   Total: \( M(l-1) \cdot (M(l-1)-M(l-2)) \cdot A_2 \).
 *
 * @param level Maximum depth of trees (non‑negative integer).
 * @return Total number of trees with depth ≤ level.
 */
    [[nodiscard]] SerialNumber_t MaxSerialNumber(std::size_t level) const
    {
        // Base case: only leaves (arity 0) have depth 0.
        if (level == 0) {
            return m_atoms->arg0.size();
        }

        // Number of trees with depth ≤ level-1.
        const auto max_prev = MaxSerialNumber(level - 1);

        // Number of trees with depth exactly level-1.
        // For level = 1, trees of depth exactly 0 are just the leaves,
        // and M(-1) is defined as 0.
        const auto max_prev_lvl = (level > 1) ? max_prev - MaxSerialNumber(level - 2) : max_prev;

        // Total for depth ≤ level:
        //   old ones (max_prev)
        // + new unary trees of depth level
        // + new binary trees of depth level
        const auto m = (max_prev * max_prev_lvl * m_atoms->arg2.size())  // binary trees
                       + (max_prev_lvl * m_atoms->arg1.size())           // unary trees
                       + max_prev;                                       // trees of smaller depth
        return m;
    }

    /**
 * @brief Compute the unique serial number of this tree.
 *
 * Serial numbers are assigned in lexicographic order of tree structures:
 * first by increasing depth, then within each depth level according to a
 * canonical ordering:
 * - Leaves (arity 0) are ordered by their atom index (0 … A₀‑1).
 * - Unary trees are ordered first by the atom index of their root,
 *   then by the serial number of the (only) subtree.
 * - Binary trees respect the canonical form: the right subtree always has
 *   depth exactly one less than the whole tree. They are ordered first by
 *   the atom index of the root, then by the **right** subtree (among trees
 *   of exact depth level‑1), and finally by the **left** subtree (among all
 *   trees of depth ≤ level‑1).
 *
 * The numbering is built as an offset from all trees of smaller depth,
 * plus a local index within the current depth level.
 *
 * Let:
 * - \( l \) = depth of this tree (obtained via `CurrentMaxLevel()`).
 * - \( M(d) \) = `MaxSerialNumber(d)`.
 * - \( A_1 = |\text{arg1}| \), \( A_2 = |\text{arg2}| \).
 * - \( \text{idx}_1 \) = index of the root atom among unary atoms.
 * - \( \text{idx}_2 \) = index of the root atom among binary atoms.
 *
 * Then:
 * - For a leaf (arity 0):
 *   \[
 *   \text{sn} = \text{atom index}
 *   \]
 *
 * - For a unary tree:
 *   \[
 *   \text{sn} = M(l-1) \;+\; \bigl(M(l-1)-M(l-2)\bigr) \cdot \text{idx}_1
 *               \;+\; \bigl(\text{sn}(\text{sub}) - M(l-2)\bigr)
 *   \]
 *   where \(\text{sn}(\text{sub})\) is the serial number of the subtree,
 *   and \(M(l-2)\) is subtracted because the subtree must have depth exactly
 *   \(l-1\) (its serial number lies in the range \([M(l-2),\, M(l-1)-1]\)).
 *
 * - For a binary tree:
 *   \[
 *   \begin{aligned}
 *   \text{sn} = M(l-1) &+ \bigl(M(l-1)-M(l-2)\bigr) \cdot A_1 \\
 *                       &+ M(l-1)\cdot\bigl(M(l-1)-M(l-2)\bigr) \cdot \text{idx}_2 \\
 *                       &+ M(l-1) \cdot \bigl(\text{sn}(\text{right}) - M(l-2)\bigr) \\
 *                       &+ \text{sn}(\text{left})
 *   \end{aligned}
 *   \]
 *   Here \(\text{sn}(\text{right})\) is the serial number of the right subtree
 *   (depth exactly \(l-1\)) and \(\text{sn}(\text{left})\) is that of the left
 *   subtree (any depth ≤ \(l-1\)).
 *
 * @return Serial number uniquely identifying this tree.
 */
    [[nodiscard]] SerialNumber_t SerialNumber() const
    {
        // Leaf case: depth 0, number is simply the atom's index.
        if (Arity() == 0) {
            return m_atom_index.num;
        }

        // Determine the depth of this tree.
        const auto level = CurrentMaxLevel();

        // Total count of trees with depth ≤ level-1 (they occupy the first slots).
        const auto max_prev = MaxSerialNumber(level - 1);

        // Total count of trees with depth ≤ level-2 (used for normalization).
        const auto max_prev2 = (level > 1) ? MaxSerialNumber(level - 2) : 0;

        // Number of trees with depth exactly level-1.
        const auto max_prev_lvl = max_prev - max_prev2;

        // Start with the offset of all smaller-depth trees.
        std::size_t snum = max_prev;

        if (Arity() == 1) {
            // Unary tree: add contribution of the unary atom index,
            // then the normalized serial number of the subtree (which must
            // have depth exactly level-1).
            snum += max_prev_lvl * m_atom_index.num;          // offset for this atom
            auto snum1 = m_arg1->SerialNumber() - max_prev2;  // subtree index within depth level-1
            snum += snum1;
        }
        else if (Arity() == 2) {
            // Binary tree: first skip all unary trees of this depth,
            // then add the contribution of the binary atom index,
            // then add the contributions of the two subtrees.
            snum += max_prev_lvl * m_atoms->arg1.size();         // all unary trees of depth level
            snum += max_prev * max_prev_lvl * m_atom_index.num;  // offset for this binary atom
            auto snum1 = m_arg1->SerialNumber();                 // left subtree (any depth ≤ level-1)
            auto snum2 = m_arg2->SerialNumber() - max_prev2;     // right subtree (depth exactly level-1)
            snum += max_prev * snum2 + snum1;                    // combine: first by right, then by left
        }
        return snum;
    }

    void FromSerialNumber(const SerialNumber_t snum)
    {
        std::size_t level = 0;

        auto snum_l = MaxSerialNumber(level);
        while (snum_l <= snum) {
            level += 1;
            snum_l = MaxSerialNumber(level);
        }

        if (level == 0) {
            assert(snum < m_atoms->arg0.size());
            m_atom_index.arity = 0;
            m_atom_index.num = snum;
            return;
        }

        const auto max_prev = MaxSerialNumber(level - 1);
        const auto max_prev2 = (level > 1) ? MaxSerialNumber(level - 2) : 0;
        const auto max_prev_lvl = max_prev - max_prev2;
        const auto offset = snum - max_prev;
        const auto arg1_max_snum = max_prev_lvl * m_atoms->arg1.size();
        if (offset < arg1_max_snum) {
            m_atom_index.arity = 1;
            m_atom_index.num = offset / max_prev_lvl;
            const auto arg1_snum = (offset % max_prev_lvl) + max_prev2;
            m_arg1 = std::make_unique<FuncNode>(m_atoms);
            m_arg1->FromSerialNumber(arg1_snum);
        }
        else {
            m_atom_index.arity = 2;
            const auto ar2_offset = offset - max_prev_lvl * m_atoms->arg1.size();
            const auto foo = ar2_offset / max_prev;
            const auto arg1_sn = ar2_offset % max_prev;
            m_atom_index.num = foo / max_prev_lvl;
            const auto arg2_sn = foo % max_prev_lvl + max_prev2;
            m_arg1 = std::make_unique<FuncNode>(m_atoms);
            m_arg1->FromSerialNumber(arg1_sn);
            m_arg2 = std::make_unique<FuncNode>(m_atoms);
            m_arg2->FromSerialNumber(arg2_sn);
        }
    }

    /// @brief Clear cached calculation results
    void ClearCalculated() { m_values.clear(); }

    /**
     * @brief Calculate function values for all inputs
     * @param recalculate Force recalculation even if cached
     * @return Vector of output values
     * 
     * Results are cached for subsequent calls unless recalculate is true.
     */
    const FuncValues_t& Calculate(bool recalculate = false)
    {
        if (m_values.empty() or recalculate) {
            switch (Arity()) {
                case 0:
                    m_values = m_atoms->arg0[m_atom_index.num]->Calculate();
                    break;
                case 1:
                    m_values = m_atoms->arg1[m_atom_index.num]->Calculate(m_arg1->Calculate());
                    break;
                case 2:
                    m_values = m_atoms->arg2[m_atom_index.num]->Calculate(m_arg1->Calculate(), m_arg2->Calculate());
                    break;
                default:
                    break;
            }
            auto result = std::ranges::minmax_element(m_values);
            m_ch.min = *result.min;
            m_ch.max = *result.max;
        }
        return m_values;
    }

    const Characteristics<FuncValue_t>& Chars() const
    {
        assert(not m_values.empty());
        return m_ch;
    }

    /**
     * @brief Check if the entire expression is constant
     * @return true if expression evaluates to constant value for all inputs
     */
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

    /**
     * @brief Get string representation of the expression
     * @param append Optional string to append to representation
     * @return String in format f(arg1, arg2, ...)
     */
    [[nodiscard]] std::string Repr(std::string_view append = "") const
    {
        switch (Arity()) {
            case 0:
                return std::format("{}{}", m_atoms->arg0[m_atom_index.num]->Str(), append);
            case 1:
                return std::format("{}({}){}", m_atoms->arg1[m_atom_index.num]->Str(), m_arg1->Repr(), append);
            case 2:
                return std::format("{}({};{}){}", m_atoms->arg2[m_atom_index.num]->Str(), m_arg1->Repr(),
                                   m_arg2->Repr(), append);
            default:
                assert(false);
        }
        return {};
    }

    /**
     * @brief Convert tree to JSON representation
     * @return JSON object representing the tree structure
     */
    [[nodiscard]] json ToJSON() const
    {
        json j;
        j["arity"] = m_atom_index.arity;
        j["num"] = m_atom_index.num;
        j["name"] = m_atoms->Get(m_atom_index.arity, m_atom_index.num)->Str();
        if (Arity() > 0) {
            j["arg1"] = m_arg1->ToJSON();
        }
        if (Arity() > 1) {
            j["arg2"] = m_arg2->ToJSON();
        }
        return j;
    }

    /**
     * @brief Load tree from JSON representation
     * @param j JSON object containing tree structure
     * @return true if successful, false on error
     */
    bool FromJSON(const json& j_root)
    {
        m_atom_index = AtomIndex{};
        m_arg1 = nullptr;
        m_arg2 = nullptr;

        // Parse arity
        const auto j_arity = j_root.find("arity");
        if (j_arity == j_root.end()) {
            return false;
        }
        if (not j_arity->is_number_unsigned()) {
            return false;
        }
        m_atom_index.arity = j_arity->get<std::size_t>();

        // Parse function index
        const auto j_num = j_root.find("num");
        if (j_num == j_root.end()) {
            return false;
        }
        if (not j_num->is_number_unsigned()) {
            return false;
        }
        m_atom_index.num = j_num->get<std::size_t>();

        // Parse children recursively
        if (Arity() > 0) {
            m_arg1 = std::make_unique<FuncNode>(m_atoms);
            const auto j_arg1 = j_root.find("arg1");
            if (j_arg1 == j_root.end()) {
                return false;
            }
            if (not j_arg1->is_object()) {
                return false;
            }
            if (not m_arg1->FromJSON(*j_arg1)) {
                return false;
            }
        }

        if (Arity() > 1) {
            m_arg2 = std::make_unique<FuncNode>(m_atoms);
            const auto j_arg2 = j_root.find("arg2");
            if (j_arg2 == j_root.end()) {
                return false;
            }
            if (not j_arg2->is_object()) {
                return false;
            }
            if (not m_arg2->FromJSON(*j_arg2)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Initialize tree to minimum depth structure
     * @param max_depth Maximum depth for initialization
     * @param current_depth Current depth in recursion
     * @return true if successful
     * 
     * Creates a tree of specified depth with default functions.
     */
    bool InitDepth(const std::size_t max_depth, const std::size_t current_depth = 0)
    {
        assert(current_depth <= max_depth);
        m_arg2 = nullptr;

        if (current_depth == max_depth) {
            // Leaf node (nullary function)
            m_arg1 = nullptr;
            m_atom_index.arity = 0;
            m_atom_index.num = 0;
        }
        else {
            // Internal node (unary function by default)
            m_arg1 = std::make_unique<FuncNode>(m_atoms);
            m_arg1->InitDepth(max_depth, current_depth + 1);
            m_atom_index.arity = 1;
            m_atom_index.num = 0;
        }
        return true;
    }

    /**
     * @brief Advance to next tree in enumeration order
     * @param max_depth Maximum allowed tree depth
     * @param current_depth Current depth in recursion
     * @return true if next tree exists, false if enumeration complete
     * 
     * Enumerates all possible trees in lexicographic order.
     * Skips constant or symmetric trees based on template parameters.
     */
    bool Iterate(const std::size_t max_depth, const std::size_t current_depth = 0)
    {
        bool keep_iterate = true;
        while (keep_iterate) {
            if (not IterateRaw(max_depth, current_depth)) {
                return false;
            }

            ClearCalculated();

            if (Arity() == 0) {
                return true;
            }

            if (not SKIP_CONSTANT) {
                keep_iterate = false;
            }
            else {
                keep_iterate = Constant();
                if (not keep_iterate) {
                    Calculate(true);
                    if (Chars().min == Chars().max) {
                        keep_iterate = true;
                    }
                }
            }
        }

        return true;
    }

    bool IterateArity0(const std::size_t max_depth, const std::size_t next_depth)
    {
        if (LastArityFunc()) {
            if (next_depth > max_depth) {
                return false;
            }
            NextArity1();
        }
        else {
            ++m_atom_index.num;
        }
        return true;
    }

    bool IterateArity1(const std::size_t max_depth, const std::size_t next_depth)
    {
        bool arg1_iterated = m_arg1->Iterate(max_depth, next_depth);

        if (SKIP_CONSTANT) {
            if (arg1_iterated and (m_arg1->Arity() == 0) and (m_arg1->Constant())) {
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

    bool IterateArity2_CheckConstant(bool arg1_iterated)
    {
        if (SKIP_CONSTANT) {
            if (arg1_iterated and (m_arg1->Arity() == 0) and (m_arg1->Constant()) and (m_arg2->Arity() == 0) and
                (m_arg2->Constant())) {
                return false;
            }
        }
        return true;
    }

    bool IterateArity2_CheckSymmetric(bool arg1_iterated)
    {
        if (SKIP_SYMMETRIC) {
            if (arg1_iterated and m_atoms->arg2[m_atom_index.num]->Commutative()) {
                if (m_atoms->arg2[m_atom_index.num]->Idempotent()) {
                    if (m_arg1->SerialNumber() >= m_arg2->SerialNumber()) {
                        return false;
                    }
                }
                else {
                    if (m_arg1->SerialNumber() > m_arg2->SerialNumber()) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool IterateArity2(const std::size_t max_depth, const std::size_t next_depth)
    {
        bool arg1_iterated = m_arg1->Iterate(max_depth, next_depth);

        arg1_iterated = arg1_iterated and IterateArity2_CheckConstant(arg1_iterated);
        arg1_iterated = arg1_iterated and IterateArity2_CheckSymmetric(arg1_iterated);

        if (not arg1_iterated) {
            if (not m_arg2->Iterate(max_depth, next_depth)) {
                if (LastArityFunc()) {
                    return false;
                }
                NextArity2();
                m_arg2->InitDepth(max_depth, next_depth);
            }
            else {
                m_arg1 = std::make_unique<FuncNode>(m_atoms);
            }
        }
        return true;
    }

    bool IterateRaw(const std::size_t max_depth, const std::size_t current_depth)
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
    AtomFuncs_t* m_atoms = nullptr;  ///< Reference to atomic function library
    AtomIndex m_atom_index;          ///< Index of this node's function

    std::unique_ptr<FuncNode> m_arg1 = nullptr;  ///< First child (for arity >= 1)
    std::unique_ptr<FuncNode> m_arg2 = nullptr;  ///< Second child (for arity = 2)

    FuncValues_t m_values;
    Characteristics<FuncValue_t> m_ch;

    [[nodiscard]] bool LastArityFunc() const
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

/// @} // end of FunctionNodes group

}  // namespace fw
