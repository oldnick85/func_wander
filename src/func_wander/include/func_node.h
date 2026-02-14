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
     * @brief Calculate maximum serial number for trees of given depth
     * @param level Maximum depth of trees
     * @return Maximum possible serial number for trees of this depth
     */
    [[nodiscard]] SerialNumber_t MaxSerialNumber(std::size_t level) const
    {
        if (level == 0) {
            return m_atoms->arg0.size();
        }

        const auto max_prev = MaxSerialNumber(level - 1);
        const auto max_prev_lvl = (level > 1) ? max_prev - MaxSerialNumber(level - 2) : max_prev;
        const auto m =
            (max_prev * max_prev_lvl * m_atoms->arg2.size()) + (max_prev_lvl * m_atoms->arg1.size()) + max_prev;
        return m;
    }

    /**
     * @brief Get unique serial number for this tree
     * @return Serial number that uniquely identifies this tree structure
     * 
     * Serial numbers are assigned in lexicographic order of tree structures.
     */
    [[nodiscard]] SerialNumber_t SerialNumber() const
    {
        if (Arity() == 0) {
            return m_atom_index.num;
        }

        const auto level = CurrentMaxLevel();
        const auto max_prev = MaxSerialNumber(level - 1);
        const auto max_prev2 = (level > 1) ? MaxSerialNumber(level - 2) : 0;
        const auto max_prev_lvl = max_prev - max_prev2;
        std::size_t snum = max_prev;

        if (Arity() == 1) {
            snum += max_prev_lvl * m_atom_index.num;
            auto snum1 = m_arg1->SerialNumber() - max_prev2;
            snum += snum1;
        }
        else if (Arity() == 2) {
            snum += max_prev_lvl * m_atoms->arg1.size();
            snum += max_prev * max_prev_lvl * m_atom_index.num;
            auto snum1 = m_arg1->SerialNumber();
            auto snum2 = m_arg2->SerialNumber() - max_prev2;
            snum += max_prev * snum2 + snum1;
        }
        return snum;
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
