/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file zbdd.h
/// Zero-Suppressed Binary Decision Diagram facilities.

#ifndef SCRAM_SRC_ZBDD_H_
#define SCRAM_SRC_ZBDD_H_

#include <cstdint>

#include <array>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

#include "bdd.h"

namespace scram {
namespace core {

/// Representation of non-terminal nodes in ZBDD.
/// Complement variables are represented with negative indices.
/// The order of the complement is higher than the order of the variable.
class SetNode : public NonTerminal<SetNode> {
 public:
  using NonTerminal::NonTerminal;  ///< Constructor with index and order.

  /// @returns true if the ZBDD is minimized.
  bool minimal() const { return minimal_; }

  /// Sets the indication of a minimized ZBDD.
  ///
  /// @param[in] flag  A flag for minimized ZBDD.
  void minimal(bool flag) { minimal_ = flag; }

  /// @returns The registered order of the largest set in the ZBDD.
  int max_set_order() const { return max_set_order_; }

  /// Registers the order of the largest set in the ZBDD
  /// represented by this vertex.
  ///
  /// @param[in] order  The order/size of the largest set.
  void max_set_order(int order) { max_set_order_ = order; }

  /// @returns Whatever count is stored in this node.
  int64_t count() const { return count_; }

  /// Stores numerical value for later retrieval.
  /// This can be a helper functionality
  /// for counting the number of sets or nodes.
  ///
  /// @param[in] number  A number with a meaning for the caller.
  ///
  /// @note This field is provided for ZBDD processing techniques
  ///       that use mutually exclusive meta-data about the node
  ///       in different stages of ZBDD processing.
  ///       Usually, the data is not needed after the technique is done.
  ///       In contrast to providing separate fields or using hash tables
  ///       for each technique metric,
  ///       this general-purpose field saves space and time.
  void count(int64_t number) { count_ = number; }

  /// @returns Products found in the ZBDD represented by this node.
  const std::vector<std::vector<int>>& products() const { return products_; }

  /// Sets the products belonging to this ZBDD.
  ///
  /// @param[in] products  Products calculated from low and high edges.
  void products(const std::vector<std::vector<int>>& products) {
    products_ = products;
  }

  using NonTerminal::CutBranches;  ///< For destructive extraction of products.

  /// Recovers a shared pointer to SetNode from a pointer to Vertex.
  ///
  /// @param[in] vertex  Pointer to a Vertex known to be a SetNode.
  ///
  /// @return Casted pointer to SetNode.
  static IntrusivePtr<SetNode> Ptr(
      const IntrusivePtr<Vertex<SetNode>>& vertex) {
    return boost::static_pointer_cast<SetNode>(vertex);
  }

 private:
  bool minimal_ = false;  ///< A flag for minimized collection of sets.
  int max_set_order_ = 0;  ///< The order of the largest set in the ZBDD.
  std::vector<std::vector<int>> products_;  ///< Products of this node.
  int64_t count_ = 0;  ///< The number of products, nodes, or anything else.
};

using SetNodePtr = IntrusivePtr<SetNode>;  ///< Shared ZBDD set nodes.

/// Function for hashing a pair of ordered numbers.
struct PairHash {
  /// Operator overload for hashing two ordered numbers.
  ///
  /// @param[in] p  The pair of numbers.
  ///
  /// @returns Hash value of the pair.
  std::size_t operator()(const std::pair<int, int>& p) const noexcept {
    return boost::hash_value(p);
  }
};

/// Hash table with pairs of numbers as keys.
///
/// @tparam Value  Type of values to be stored in the table.
template <typename Value>
using PairTable = std::unordered_map<std::pair<int, int>, Value, PairHash>;

using Triplet = std::array<int, 3>;  ///< Triplet of numbers for functions.

/// Functor for hashing triplets of ordered numbers.
struct TripletHash {
  /// Operator overload for hashing three ordered numbers.
  ///
  /// @param[in] triplet  Three numbers.
  ///
  /// @returns Hash value of the triplet.
  std::size_t operator()(const Triplet& triplet) const noexcept {
    return boost::hash_range(triplet.begin(), triplet.end());
  }
};

/// Hash table with triplets of numbers as keys.
///
/// @tparam Value  Type of values to be stored in the table.
template <typename Value>
using TripletTable = std::unordered_map<Triplet, Value, TripletHash>;

/// Zero-Suppressed Binary Decision Diagrams for set manipulations.
class Zbdd {
 public:
  using VertexPtr = IntrusivePtr<Vertex<SetNode>>;  ///< ZBDD vertex base.
  using TerminalPtr = IntrusivePtr<Terminal<SetNode>>;  ///< Terminal vertex.

  /// Converts Reduced Ordered BDD
  /// into Zero-Suppressed BDD.
  ///
  /// @param[in] bdd  ROBDD with the ITE vertices.
  /// @param[in] settings  Settings for analysis.
  ///
  /// @pre BDD has attributed edges with only one terminal (1/True).
  ///
  /// @post The input BDD structure is not changed.
  ///
  /// @note The input BDD is not passed as a constant
  ///       because ZBDD needs BDD facilities to calculate prime implicants.
  ///       However, ZBDD guarantees to preserve the original BDD structure.
  Zbdd(Bdd* bdd, const Settings& settings) noexcept;

  /// Constructor with the analysis target.
  /// ZBDD is directly produced from a Boolean graph.
  ///
  /// @param[in] fault_tree  Preprocessed, partially normalized,
  ///                        and indexed fault tree.
  /// @param[in] settings  The analysis settings.
  ///
  /// @pre The passed Boolean graph already has variable ordering.
  /// @note The construction may take considerable time.
  Zbdd(const BooleanGraph* fault_tree, const Settings& settings) noexcept;

  Zbdd(const Zbdd&) = delete;
  Zbdd& operator=(const Zbdd&) = delete;

  virtual ~Zbdd() noexcept = default;

  /// Runs the analysis
  /// with the representation of a Boolean graph as ZBDD.
  ///
  /// @warning The analysis will destroy ZBDD.
  ///
  /// @post Products are generated with ZBDD analysis.
  void Analyze() noexcept;

  /// @returns Products generated by the analysis.
  const std::vector<std::vector<int>>& products() const { return products_; }

 protected:
  /// Default constructor to initialize member variables.
  ///
  /// @param[in] settings  Settings that control analysis complexity.
  /// @param[in] coherent  A flag for coherent modular functions.
  /// @param[in] module_index  The of a module if known.
  explicit Zbdd(const Settings& settings, bool coherent = false,
                int module_index = 0) noexcept;

  /// @returns Current root vertex of the ZBDD.
  const VertexPtr& root() const { return root_; }

  /// Sets a new root vertex for ZBDD.
  ///
  /// @param[in] vertex  A vertex already registered in this ZBDD.
  void root(const VertexPtr& vertex) { root_ = vertex; }

  /// @returns Analysis setting with this ZBDD.
  const Settings& settings() const { return kSettings_; }

  /// @returns A set of registered and fully processed modules;
  const std::unordered_map<int, std::unique_ptr<Zbdd>>& modules() const {
    return modules_;
  }

  /// Logs properties of the Zbdd.
  void Log() noexcept;

  /// Finds or adds a unique SetNode in the ZBDD.
  /// All vertices in the ZBDD must be created with this functions.
  /// Otherwise, the ZBDD may not be reduced,
  /// and vertices will miss crucial meta-information about the ZBDD.
  ///
  /// @param[in] index  Positive or negative index of the node.
  /// @param[in] high  The high vertex.
  /// @param[in] low  The low vertex.
  /// @param[in] order The order for the vertex variable.
  /// @param[in] module  The indication of a proxy for a modular ZBDD.
  /// @param[in] coherent  The indication of a coherent proxy.
  ///
  /// @returns Set node with the given parameters.
  ///
  /// @warning This function is not aware of reduction rules.
  SetNodePtr FindOrAddVertex(int index, const VertexPtr& high,
                             const VertexPtr& low, int order,
                             bool module = false,
                             bool coherent = false) noexcept;

  /// Find or adds a ZBDD SetNode vertex using information from gates.
  ///
  /// @param[in] gate  Gate with index, order, and other information.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  ///
  /// @returns SetNode for a replacement.
  ///
  /// @warning This function is not aware of reduction rules.
  SetNodePtr FindOrAddVertex(const GatePtr& gate, const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Applies Boolean operation to two vertices representing sets.
  /// This is the main function for the operation.
  ///
  /// @tparam Type  The operator enum.
  ///
  /// @param[in] arg_one  First argument ZBDD set.
  /// @param[in] arg_two  Second argument ZBDD set.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns The resulting ZBDD vertex.
  ///
  /// @post The limit on the set order is guaranteed.
  template <Operator Type>
  VertexPtr Apply(const VertexPtr& arg_one, const VertexPtr& arg_two,
                  int limit_order) noexcept;

  /// Applies Boolean operation to two vertices representing sets.
  /// This is a convenience function
  /// if the operator type cannot be determined at compile time.
  ///
  /// @param[in] type  The operator or type of the gate.
  /// @param[in] arg_one  First argument ZBDD set.
  /// @param[in] arg_two  Second argument ZBDD set.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns The resulting ZBDD vertex.
  ///
  /// @pre The operator is either AND or OR.
  ///
  /// @post The limit on the set order is guaranteed.
  VertexPtr Apply(Operator type, const VertexPtr& arg_one,
                  const VertexPtr& arg_two, int limit_order) noexcept;

  /// Applies Boolean operation to ZBDD graph non-terminal vertices.
  ///
  /// @tparam Type  The operator enum.
  ///
  /// @param[in] arg_one  First argument set vertex.
  /// @param[in] arg_two  Second argument set vertex.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns The resulting ZBDD vertex.
  ///
  /// @pre Argument vertices are ordered.
  template <Operator Type>
  VertexPtr Apply(const SetNodePtr& arg_one, const SetNodePtr& arg_two,
                  int limit_order) noexcept;

  /// Removes complements of variables from products.
  /// This procedure only needs to be performed for non-coherent graphs
  /// with minimal cut sets as output.
  ///
  /// @param[in] vertex  The variable vertex in the ZBDD.
  /// @param[in,out] wide_results  Memoisation of the processed vertices.
  ///
  /// @returns Processed vertex.
  ///
  /// @post Sub-modules are not processed.
  /// @post Complements of modules are not eliminated.
  VertexPtr EliminateComplements(
      const VertexPtr& vertex,
      std::unordered_map<int, VertexPtr>* wide_results) noexcept;

  /// Removes constant modules from products.
  /// Constant modules are likely to happen after complement elimination.
  /// This procedure is inherently bottom-up,
  /// so the caller must make sure
  /// that the modules have already been pre-processed.
  ///
  /// @pre All modules have been processed.
  void EliminateConstantModules() noexcept;

  /// Removes subsets in ZBDD.
  ///
  /// @param[in] vertex  The variable node in the set.
  ///
  /// @returns Processed vertex.
  VertexPtr Minimize(const VertexPtr& vertex) noexcept;

  /// Traverses ZBDD to find modules and adjusted cut-offs.
  /// Modules within modules are not gathered.
  ///
  /// @param[in] vertex  The root vertex to start with.
  /// @param[in] current_order  The product order from the top to the module.
  /// @param[in,out] modules  A map of module indices, coherence, and cut-offs.
  ///
  /// @returns The minimum product order from the bottom.
  /// @returns -1 if the vertex is terminal Empty on low branch only.
  ///
  /// @pre The ZBDD is minimal.
  int GatherModules(
      const VertexPtr& vertex,
      int current_order,
      std::unordered_map<int, std::pair<bool, int>>* modules) noexcept;

  /// Clears all memoization tables.
  void ClearTables() noexcept {
    and_table_.clear();
    or_table_.clear();
    minimal_results_.clear();
    subsume_table_.clear();
    prune_results_.clear();
  }

  /// Releases all possible memory from memoization and unique tables.
  void ReleaseTables() noexcept {
    unique_table_.Release();
    Zbdd::ClearTables();
    and_table_.reserve(0);
    or_table_.reserve(0);
    minimal_results_.reserve(0);
    subsume_table_.reserve(0);
  }

  /// Joins a ZBDD representing a module gate.
  ///
  /// @param[in] index  The index of the module.
  /// @param[in] container  The container of the module graph.
  ///
  /// @pre The module products are final,
  ///      and no more processing or sanitizing is needed.
  void JoinModule(int index, std::unique_ptr<Zbdd> container) noexcept {
    assert(!modules_.count(index));
    assert(container->root()->terminal() ||
           SetNode::Ptr(container->root())->minimal());
    modules_.emplace(index, std::move(container));
  }

  /// @todo Redesign vertex management and creation.
  ///       The management mechanism must be encapsulated.
  ///       Invariants must be private.
  const TerminalPtr kBase_;  ///< Terminal Base (Unity/1) set.
  const TerminalPtr kEmpty_;  ///< Terminal Empty (Null/0) set.

 private:
  using SetNodeWeakPtr = WeakIntrusivePtr<SetNode>;  ///< Pointer for tables.
  using ComputeTable = TripletTable<VertexPtr>;  ///< General computation table.
  using Product = std::vector<int>;  ///< For clarity of expected results.
  /// Module entry in the tables with its original gate index.
  using ModuleEntry = std::pair<const int, std::unique_ptr<Zbdd>>;

  /// Converts a modular BDD function
  /// into Zero-Suppressed BDD.
  ///
  /// @param[in] module  Modular BDD function.
  /// @param[in] coherent  A flag for coherent modular functions.
  /// @param[in] bdd  ROBDD with the ITE vertices.
  /// @param[in] settings  Settings for analysis.
  /// @param[in] module_index  The of a module if known.
  ///
  /// @pre BDD has attributed edges with only one terminal (1/True).
  ///
  /// @post The input BDD structure is not changed.
  ///
  /// @note The input BDD is not passed as a constant
  ///       because ZBDD needs BDD facilities to calculate prime implicants.
  ///       However, ZBDD guarantees to preserve the original BDD structure.
  Zbdd(const Bdd::Function& module, bool coherent, Bdd* bdd,
       const Settings& settings, int module_index = 0) noexcept;

  /// Constructs ZBDD from modular Boolean graphs.
  /// This constructor does not handle constant or single variable graphs.
  /// These cases are expected to be handled
  /// after calling this constructor.
  /// In these special cases,
  /// this constructor guarantees
  /// all initialization except for the root.
  ///
  /// @param[in] gate  The root gate of a module.
  /// @param[in] settings  Analysis settings.
  ///
  /// @post The root vertex pointer is uninitialized
  ///       if the Boolean graph is constant or single variable.
  Zbdd(const GatePtr& gate, const Settings& settings) noexcept;

  /// Finds a replacement for an existing node
  /// or adds a new node based on an existing node.
  ///
  /// @param[in] node  An existing node.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  ///
  /// @returns Set node for a replacement.
  ///
  /// @warning This function is not aware of reduction rules.
  SetNodePtr FindOrAddVertex(const SetNodePtr& node, const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Adds a new or finds an existing reduced ZBDD vertex
  /// with parameters of a prototype BDD ITE vertex.
  ///
  /// @param[in] ite  The prototype BDD ITE vertex.
  /// @param[in] complement  Vertex represents complement of a variable.
  /// @param[in] high  The high ZBDD vertex.
  /// @param[in] low  The low ZBDD vertex.
  ///
  /// @returns Resultant reduced vertex.
  VertexPtr GetReducedVertex(const ItePtr& ite, bool complement,
                             const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Finds a replacement reduced vertex for an existing node,
  /// or adds a new node based on an existing node.
  ///
  /// @param[in] node  An existing node.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  ///
  /// @returns Set node for a replacement.
  VertexPtr GetReducedVertex(const SetNodePtr& node,
                             const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Computes the key for computation results.
  /// The key is used in computation memoisation tables.
  ///
  /// @param[in] arg_one  First argument.
  /// @param[in] arg_two  Second argument.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns A triplet of integers for the computation key.
  ///
  /// @pre The arguments are not the same functions.
  ///      Equal ID functions are handled by the reduction.
  /// @pre Even though the arguments are not SetNodePtr type,
  ///      they are ZBDD SetNode vertices.
  Triplet GetResultKey(const VertexPtr& arg_one, const VertexPtr& arg_two,
                       int limit_order) noexcept;

  /// Converts BDD graph into ZBDD graph.
  ///
  /// @param[in] vertex  Vertex of the ROBDD graph.
  /// @param[in] complement  Interpretation of the vertex as complement.
  /// @param[in] bdd_graph  The main ROBDD as helper database.
  /// @param[in] limit_order  The maximum size of requested sets.
  /// @param[in,out] ites  Processed function graphs with ids and limit order.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  ///
  /// @post The input BDD structure is not changed.
  VertexPtr ConvertBdd(const Bdd::VertexPtr& vertex, bool complement,
                       Bdd* bdd_graph, int limit_order,
                       PairTable<VertexPtr>* ites) noexcept;

  /// Converts BDD if-then-else vertex into ZBDD graph.
  /// This overload differs in that
  /// it does not register the results.
  /// It is used by the BDD vertex to ZBDD converter,
  /// and this function should not be called directly.
  ///
  /// @param[in] ite  ITE vertex of the ROBDD graph.
  /// @param[in] complement  Interpretation of the vertex as complement.
  /// @param[in] bdd_graph  The main ROBDD as helper database.
  /// @param[in] limit_order  The maximum size of requested sets.
  /// @param[in,out] ites  Processed function graphs with ids and limit order.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  VertexPtr ConvertBdd(const ItePtr& ite, bool complement,
                       Bdd* bdd_graph, int limit_order,
                       PairTable<VertexPtr>* ites) noexcept;

  /// Converts BDD if-then-else vertex into ZBDD graph for prime implicants.
  /// This is used by the BDD vertex to ZBDD converter,
  /// and this function should not be called directly.
  ///
  /// @param[in] ite  ITE vertex of the ROBDD graph.
  /// @param[in] complement  Interpretation of the vertex as complement.
  /// @param[in] bdd_graph  The main ROBDD as helper database.
  /// @param[in] limit_order  The maximum size of requested sets.
  /// @param[in,out] ites  Processed function graphs with ids and limit order.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  VertexPtr ConvertBddPrimeImplicants(const ItePtr& ite, bool complement,
                                      Bdd* bdd_graph, int limit_order,
                                      PairTable<VertexPtr>* ites) noexcept;

  /// Transforms a Boolean graph gate into a Zbdd set graph.
  ///
  /// @param[in] gate  The root gate of the Boolean graph.
  /// @param[in,out] gates  Processed gates with use counts.
  /// @param[out] module_gates  Sub-module gates.
  ///
  /// @returns The top vertex of the Zbdd graph.
  ///
  /// @pre The memoization container is not used outside of this function.
  ///
  /// @post Sub-module gates are not processed.
  VertexPtr ConvertGraph(
      const GatePtr& gate,
      std::unordered_map<int, std::pair<VertexPtr, int>>* gates,
      std::unordered_map<int, GatePtr>* module_gates) noexcept;

  /// Processes complements in a SetNode with processed high/low edges.
  ///
  /// @param[in] node  SetNode to be processed.
  /// @param[in] high  Processed high edge.
  /// @param[in] low  Processed low edge.
  ///
  /// @returns Processed ZBDD vertex without complements.
  ///
  /// @post Sub-modules are not processed.
  /// @post Complements of modules are not eliminated.
  VertexPtr EliminateComplement(const SetNodePtr& node, const VertexPtr& high,
                                const VertexPtr& low) noexcept;

  /// Removes constant modules from products.
  ///
  /// @param[in] vertex  The variable vertex in the ZBDD.
  /// @param[in,out] results  Memoisation of the processed vertices.
  ///
  /// @returns Processed vertex.
  ///
  /// @pre All sub-modules are already processed.
  VertexPtr EliminateConstantModules(
      const VertexPtr& vertex,
      std::unordered_map<int, VertexPtr>* results) noexcept;

  /// Processes constant modules in a SetNode with processed high/low edges.
  ///
  /// @param[in] node  SetNode to be processed.
  /// @param[in] high  Processed high edge.
  /// @param[in] low  Processed low edge.
  ///
  /// @returns Processed ZBDD vertex without constant modules.
  ///
  /// @pre All sub-modules are already processed.
  VertexPtr EliminateConstantModule(const SetNodePtr& node,
                                    const VertexPtr& high,
                                    const VertexPtr& low) noexcept;

  /// Applies subsume operation on two sets.
  /// Subsume operation removes
  /// paths that exist in Low branch from High branch.
  ///
  /// @param[in] high  True/then/high branch of a variable.
  /// @param[in] low  False/else/low branch of a variable.
  ///
  /// @returns Minimized high branch for a variable.
  VertexPtr Subsume(const VertexPtr& high, const VertexPtr& low) noexcept;

  /// Prunes the ZBDD graph with the cut-off.
  ///
  /// @param[in] vertex  The root vertex of the ZBDD.
  /// @param[in] limit_order  The cut-off order for the sets.
  ///
  /// @returns The root vertex of the pruned ZBDD.
  ///
  /// @post If the ZBDD is minimal,
  ///       the resultant pruned ZBDD is minimal.
  VertexPtr Prune(const VertexPtr& vertex, int limit_order) noexcept;

  /// Checks if a set node represents a gate.
  /// Apply operations and truncation operations
  /// should avoid accounting non-module gates
  /// if they exist in ZBDD.
  ///
  /// @param[in] node  A node to be tested.
  ///
  /// @returns true for modules by default.
  virtual bool IsGate(const SetNodePtr& node) noexcept {
    return node->module();
  }

  /// Checks if a node have a possibility to represent Unity.
  ///
  /// @param[in] node  SetNode to test for possibility of Unity.
  ///
  /// @returns false if the passed node can never be Unity.
  bool MayBeUnity(const SetNodePtr& node) noexcept;

  /// Encodes the limit order for sub-sets for product generation.
  /// The encoding lets avoid generation of unnecessary products.
  ///
  /// @param[in,out] vertex  The vertex to start the encoding.
  /// @param[in] limit_order  The limit on the product order.
  ///
  /// @pre 'count' fields of nodes are clear.
  /// @pre All processing is done,
  ///      such minimization and constant module elimination.
  /// @pre The encoding is done before the product generation.
  ///
  /// @post The limit order is encoded in the node count field.
  /// @post The encoding is not propagated to sub-modules.
  void EncodeLimitOrder(const VertexPtr& vertex, int limit_order) noexcept;

  /// Traverses the reduced ZBDD graph to generate products.
  /// ZBDD is destructively converted into products.
  ///
  /// @param[in] vertex  The root node in traversal.
  ///
  /// @returns A collection of products
  ///          generated from the ZBDD subgraph.
  ///
  /// @pre The ZBDD node marks are clear.
  /// @pre The ZBDD is minimized.
  /// @pre There are no constant modules.
  /// @pre The internal limit order for generation
  ///      is encoded in the node counts.
  ///
  /// @post The products of modules are incorporated to the result.
  ///
  /// @warning Product generation will destroy ZBDD.
  std::vector<std::vector<int>>
  GenerateProducts(const VertexPtr& vertex) noexcept;

  /// Counts the number of SetNodes
  /// excluding the nodes in the modules.
  ///
  /// @param[in] vertex  The root vertex to start counting.
  ///
  /// @returns The total number of SetNode vertices
  ///          including vertices in modules.
  ///
  /// @pre SetNode marks are clear (false).
  int CountSetNodes(const VertexPtr& vertex) noexcept;

  /// Counts the total number of sets in ZBDD.
  ///
  /// @param[in] vertex  The root vertex of ZBDD.
  /// @param[in] modules  Unroll sets with modules.
  ///
  /// @returns The number of products in ZBDD.
  ///
  /// @pre SetNode marks are clear (false).
  int64_t CountProducts(const VertexPtr& vertex, bool modules) noexcept;

  /// Cleans up non-terminal vertex marks
  /// by setting them to "false".
  ///
  /// @param[in] vertex  The root vertex of the graph.
  /// @param[in] modules  Clear marks in modules as well.
  ///
  /// @pre The graph is marked "true" contiguously.
  void ClearMarks(const VertexPtr& vertex, bool modules) noexcept;

  /// Cleans up non-terminal vertex count fields
  /// by setting them to 0.
  ///
  /// @param[in] vertex  The root vertex of the graph.
  /// @param[in] modules  Clear counts in modules as well.
  ///
  /// @pre The graph node marks are clear.
  ///
  /// @post Traversed marks are set to 'true'.
  void ClearCounts(const VertexPtr& vertex, bool modules) noexcept;

  /// Checks ZBDD graphs for errors in the structure.
  /// Errors are assertions that fail at runtime.
  ///
  /// @param[in] vertex  The root vertex of ZBDD.
  /// @param[in] modules  Test modules as well.
  ///
  /// @pre SetNode marks are clear (false).
  void TestStructure(const VertexPtr& vertex, bool modules) noexcept;

  const Settings kSettings_;  ///< Analysis settings.
  VertexPtr root_;  ///< The root vertex of ZBDD.
  bool coherent_;  ///< Inherited coherence from BDD.
  int module_index_;  ///< Identifier for a module if any.

  /// Table of unique SetNodes denoting sets.
  /// The key consists of (index, id_high, id_low) triplet.
  UniqueTable<SetNode> unique_table_;

  /// Table of processed computations over sets.
  /// The argument sets are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  /// The key is {min_id, max_id, max_order}.
  /// @{
  ComputeTable and_table_;
  ComputeTable or_table_;
  /// @}

  /// Memoization of minimal ZBDD vertices.
  std::unordered_map<int, VertexPtr> minimal_results_;
  /// The results of subsume operations over sets.
  PairTable<VertexPtr> subsume_table_;
  /// The results of pruning operations.
  PairTable<VertexPtr> prune_results_;

  std::unordered_map<int, std::unique_ptr<Zbdd>> modules_;  ///< Module graphs.
  int set_id_;  ///< Identification assignment for new set graphs.
  std::vector<Product> products_;  ///< Generated products.
};

namespace zbdd {

/// Storage for generated cut sets in MOCUS.
/// The semantics is similar to a set of cut sets.
/// The container assumes special variable ordering.
/// Gates to be processed must be ordered to the top.
class CutSetContainer : public Zbdd {
 public:
  /// Default constructor to initialize member variables.
  ///
  /// @param[in] settings  Settings that control analysis complexity.
  /// @param[in] module_index  The of a module if known.
  /// @param[in] gate_index_bound  The exclusive lower bound for gate indices.
  ///
  /// @pre No complements of gates.
  /// @pre Gates are indexed sequentially
  ///      starting from a number larger than the lower bound.
  /// @pre Basic events are indexed sequentially
  ///      up to a number less than or equal to the given lower bound.
  CutSetContainer(const Settings& settings, int module_index,
                  int gate_index_bound) noexcept;

  /// Converts a Boolean graph gate into intermediate cut sets.
  ///
  /// @param[in] gate  The target AND/OR gate with arguments.
  ///
  /// @returns The root vertex of the ZBDD representing the gate cut sets.
  VertexPtr ConvertGate(const GatePtr& gate) noexcept;

  /// Finds a gate in intermediate cut sets.
  ///
  /// @returns The index of the gate in intermediate cut sets.
  /// @returns 0 if no gates are found.
  ///
  /// @pre Variable ordering puts the gate to the top (root).
  int GetNextGate() noexcept {
    if (Zbdd::root()->terminal()) return 0;
    SetNodePtr node = SetNode::Ptr(Zbdd::root());
    return CutSetContainer::IsGate(node) && !node->module() ? node->index() : 0;
  }

  /// Extracts (removes!) intermediate cut sets
  /// containing a node with a given index.
  ///
  /// @param[in] index  The index of the gate.
  ///
  /// @returns The root of the ZBDD containing the intermediate cut sets.
  ///
  /// @pre Variable ordering puts the gate to the top (root).
  ///
  /// @post The extracted cut sets are pre-processed
  ///       by removing the vertex with the index of the gate.
  VertexPtr ExtractIntermediateCutSets(int index) noexcept;

  /// Expands the intermediate ZBDD representation of a gate
  /// in intermediate cut sets containing the gate.
  ///
  /// @param[in] gate_zbdd  The intermediate ZBDD of the gate.
  /// @param[in] cut_sets  A collection of cut sets.
  ///
  /// @returns The root vertex of the resulting ZBDD.
  ///
  /// @pre The intermediate cut sets are pre-processed
  ///      by removing the vertex with the index of the gate.
  VertexPtr ExpandGate(const VertexPtr& gate_zbdd,
                       const VertexPtr& cut_sets) noexcept;

  /// Merges a set of cut sets into the main container.
  ///
  /// @param[in] vertex  The root ZBDD vertex representing the cut sets.
  ///
  /// @pre The argument ZBDD cut sets are managed by this container.
  void Merge(const VertexPtr& vertex) noexcept;

  /// Eliminates all complements from cut sets.
  /// This can only be done
  /// if the cut set generation is certain not to have conflicts.
  ///
  /// @pre The cut sets have negative literals, i.e., non-coherent.
  ///
  /// @post Sub-modules are not processed.
  void EliminateComplements() noexcept {
    std::unordered_map<int, VertexPtr> wide_results;
    Zbdd::root(Zbdd::EliminateComplements(Zbdd::root(), &wide_results));
  }

  /// Removes constant modules from cut sets.
  /// Constant modules are likely to happen after complement elimination.
  ///
  /// @pre All modules have been processed.
  void EliminateConstantModules() noexcept { Zbdd::EliminateConstantModules(); }

  /// Minimizes cut sets in the container.
  ///
  /// @post Sub-modules are not processed.
  void Minimize() noexcept { Zbdd::root(Zbdd::Minimize(Zbdd::root())); }

  /// Gathers all module indices in the cut sets.
  ///
  /// @returns An unordered map module of indices, coherence, and cut-offs.
  std::unordered_map<int, std::pair<bool, int>> GatherModules() noexcept {
    assert(Zbdd::modules().empty() && "Unexpected call with defined modules?!");
    std::unordered_map<int, std::pair<bool, int>> modules;
    Zbdd::GatherModules(Zbdd::root(), 0, &modules);
    return modules;
  }

  using Zbdd::JoinModule;  ///< Joins fully processed modules.
  using Zbdd::Log;  ///< Logs properties of the container.

 private:
  /// Checks if a set node represents a gate.
  ///
  /// @param[in] node  A node to be tested.
  ///
  /// @returns true if the index of the node belongs to a gate.
  ///
  /// @pre There are no complements of gates.
  /// @pre Gate indexation has a lower bound.
  bool IsGate(const SetNodePtr& node) noexcept override {
    return node->index() > gate_index_bound_;
  }

  int gate_index_bound_;  ///< The exclusive lower bound for the gate indices.
};

}  // namespace zbdd

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_ZBDD_H_
