/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file preprocessor.h
/// A collection of PDAG transformation/preprocessing algorithms
/// that simplify fault trees for analysis.

#ifndef SCRAM_SRC_PREPROCESSOR_H_
#define SCRAM_SRC_PREPROCESSOR_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>

#include "pdag.h"

namespace scram {
namespace core {

namespace pdag {

/// The terminal case for graph transformations.
inline void Transform(Pdag* /*graph*/) {}

/// Applies graph transformations consecutively.
///
/// @param[in,out] graph  The graph to be transformed.
/// @param[in] unary_op  The first operation to be applied.
/// @param[in] unary_ops  The rest of transformations to apply.
template <typename T, typename... Ts>
void Transform(Pdag* graph, T&& unary_op, Ts&&... unary_ops) noexcept {
  if (graph->IsTrivial())
    return;
  unary_op(graph);
  Transform(graph, std::forward<Ts>(unary_ops)...);
}

/// Determines the order of traversal for gate arguments.
/// This function does not assign the order of nodes.
///
/// @tparam T  Type of the arguments.
///
/// @param[in] gate  The host gate parent.
///
/// @returns An ordered, stable list of arguments.
template <class T>
std::vector<T*> OrderArguments(Gate* gate) noexcept;

/// Assigns topological ordering to nodes of the PDAG.
/// The ordering is assigned to the node order marks.
/// The nodes are sorted in descending optimization value.
/// The highest order value belongs to the root.
///
/// @param[in,out] graph  The graph to be processed.
///
/// @post The root and descendant node order marks contain the ordering.
void TopologicalOrder(Pdag* graph) noexcept;

/// Marks coherence of the whole graph.
///
/// @param[in,out] graph  The graph to be processed.
///
/// @post Gate traversal marks are dirty.
void MarkCoherence(Pdag* graph) noexcept;

}  // namespace pdag

/// The class provides main preprocessing operations
/// over a PDAG
/// to simplify the fault tree
/// and to help analysis run more efficiently.
class Preprocessor : private boost::noncopyable {
 public:
  /// Constructs a preprocessor of a PDAG
  /// representing a fault tree.
  ///
  /// @param[in] graph  The PDAG to be preprocessed.
  ///
  /// @warning There should not be another shared pointer to the root gate
  ///          outside of the passed PDAG.
  ///          Upon preprocessing a new root gate may be assigned to the graph,
  ///          and, if there is an extra pointer to the previous top gate
  ///          outside of the graph,
  ///          the destructor will not be called
  ///          as expected by the preprocessing algorithms,
  ///          which will mess the new structure of the PDAG.
  explicit Preprocessor(Pdag* graph) noexcept;

  virtual ~Preprocessor() = default;

  /// Runs the graph preprocessing.
  void operator()() noexcept;

 protected:
  class GateSet;  ///< Container of unique gates by semantics.

  /// Runs the default preprocessing
  /// that achieves the graph in a normal form.
  virtual void Run() noexcept = 0;

  /// The initial phase of preprocessing.
  /// The most basic cleanup algorithms are applied.
  /// The cleanup should benefit all other phases
  /// and algorithms.
  ///
  /// @note The constants or house events in the graph are cleaned.
  ///       Any constant state gates must be removed
  ///       by the future preprocessing algorithms
  ///       as they introduce these constant state  gates.
  /// @note NULL type (pass-through) gates are processed.
  ///       Any NULL type gates must be processed and removed
  ///       by the future preprocessing algorithms
  ///       as they introduce these NULL type gates.
  ///
  /// @warning This phase also runs partial normalization of gates;
  ///          however, the preprocessing algorithms should not rely on this.
  ///          If the partial normalization messes some significant algorithm,
  ///          it may be removed from this phase in future.
  void RunPhaseOne() noexcept;

  /// Preprocessing phase of the original structure of the graph.
  /// This phase attempts to leverage
  /// the existing information from the structure of the graph
  /// to maximize the optimization
  /// and to make the preprocessing techniques efficient.
  ///
  /// @note Multiple definitions of gates are detected and processed.
  /// @note Modules are detected and created.
  /// @note Non-module and non-multiple gates are coalesced.
  /// @note Boolean optimization is applied.
  void RunPhaseTwo() noexcept;

  /// Application of gate normalization.
  /// After this phase,
  /// the graph is in normal form.
  ///
  /// @note Gate normalization is conducted.
  void RunPhaseThree() noexcept;

  /// Propagation of complements.
  /// Complements are propagated down to the variables in the graph.
  /// After this phase,
  /// the graph is in negation normal form.
  ///
  /// @note Complements are propagated to the variables of the graph.
  void RunPhaseFour() noexcept;

  /// The final phase
  /// that cleans up the graph,
  /// and puts the structure of the graph ready for analysis.
  /// This phase makes the graph structure
  /// alternating AND/OR gate layers.
  void RunPhaseFive() noexcept;

  /// Normalizes the gates of the whole PDAG
  /// into OR, AND gates.
  ///
  /// @param[in] full  A flag to handle complex gates like XOR and K/N,
  ///                  which generate a lot more new gates
  ///                  and make the structure of the graph more complex.
  ///
  /// @note The negation of the top gate is saved
  ///       and handled as a special case for negation propagation
  ///       because it does not have a parent.
  /// @note New gates are created only upon full normalization
  ///       of complex gates like XOR and K/N.
  /// @note The full normalization is meant to be called only once.
  ///
  /// @warning The root get may still be NULL type.
  /// @warning Gate marks are used.
  /// @warning Node ordering may be used for full normalization.
  /// @warning Node visit information is used.
  void NormalizeGates(bool full) noexcept;

  /// Notifies all parents of negative gates,
  /// such as NOT, NOR, and NAND,
  /// before transforming these gates
  /// into basic gates of OR and AND.
  /// The argument gates are swapped with a negative sign.
  ///
  /// @param[in] gate  The gate to start processing.
  ///
  /// @note This function is a helper function for NormalizeGates().
  ///
  /// @warning Gate marks must be clear.
  /// @warning This function does not change the types of gates.
  /// @warning The root gate does not have parents,
  ///          so it is not handled here.
  void NotifyParentsOfNegativeGates(const GatePtr& gate) noexcept;

  /// Normalizes complex gates into OR, AND gates.
  ///
  /// @param[in,out] gate  The gate to be processed.
  /// @param[in] full  A flag to handle complex gates like XOR and K/N.
  ///
  /// @note This is a helper function for NormalizeGates().
  ///
  /// @note This function registers NULL type gates for future removal.
  ///
  /// @warning Gate marks must be clear.
  /// @warning The parents of negative gates are assumed to be
  ///          notified about the change of their arguments' types.
  void NormalizeGate(const GatePtr& gate, bool full) noexcept;

  /// Normalizes a gate with XOR logic.
  /// This is a helper function
  /// for the main gate normalization function.
  ///
  /// @param[in,out] gate  The gate to normalize.
  ///
  /// @note This is a helper function for NormalizeGate.
  void NormalizeXorGate(const GatePtr& gate) noexcept;

  /// Normalizes a VOTE gate with a vote number.
  /// The gate is turned into an OR gate of
  /// recursively normalized VOTE and AND arg gates
  /// according to the formula
  /// K/N(x, y_i) = OR(AND(x, K-1/N-1(y_i)), K/N-1(y_i)))
  /// with y_i being the rest of formula arguments,
  /// which exclude x.
  /// This representation is more friendly
  /// to other preprocessing and analysis techniques
  /// than the alternative,
  /// which is OR of AND gates of combinations.
  /// Normalization of K/N gates is aware of variable ordering.
  ///
  /// @param[in,out] gate  The VOTE gate to normalize.
  ///
  /// @pre Variable ordering is assigned to arguments.
  /// @pre This helper function is called from NormalizeGate.
  void NormalizeVoteGate(const GatePtr& gate) noexcept;

  /// Propagates complements of argument gates down to leafs
  /// according to the De Morgan's law
  /// in order to remove any negative logic from the graph's gates.
  /// The resulting graph will contain only positive gates, OR and AND types.
  /// After this function, the PDAG is in negation normal form.
  ///
  /// @param[in,out] gate  The starting gate to traverse the graph.
  ///                      This is for recursive purposes.
  /// @param[in] keep_modules  A flag to NOT propagate complements to modules.
  /// @param[in,out] complements  The processed complements of shared gates.
  ///
  /// @note The graph must be normalized.
  ///       It must contain only OR and AND gates.
  ///
  /// @warning Gate marks must be clear.
  /// @warning If the root gate has a negative sign,
  ///          it must be handled before calling this function.
  ///          The arguments and type of the gate
  ///          must be inverted according to the logic of the root gate.
  void PropagateComplements(
      const GatePtr& gate,
      bool keep_modules,
      std::unordered_map<int, GatePtr>* complements) noexcept;

  /// Runs gate coalescence on the whole PDAG.
  ///
  /// @param[in] common  A flag to also coalesce common/shared gates.
  ///                    These gates may be important for other algorithms.
  ///
  /// @returns true if the graph has been changed.
  ///
  /// @note Module gates are omitted from coalescing to preserve them.
  ///
  /// @warning Gate marks are used.
  bool CoalesceGates(bool common) noexcept;

  /// Coalesces positive argument gates
  /// with the same OR or AND logic as parents.
  /// This function merges similar logic gates of NAND and NOR as well.
  ///
  /// @param[in,out] gate  The starting gate to traverse the graph.
  ///                      This is for recursive purposes.
  /// @param[in] common  A flag to also join common gates.
  ///                    These gates may be important for other algorithms.
  ///
  /// @returns true if the given graph has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @note Constant state gates may be generated upon joining.
  ///       These gates are registered for future processing.
  /// @note It is impossible that this function generates NULL type gates.
  /// @note Module gates are omitted from coalescing to preserve them.
  ///
  /// @warning Gate marks are used.
  bool CoalesceGates(const GatePtr& gate, bool common) noexcept;

  /// Detects and replaces multiple definitions of gates.
  /// Gates with the same logic and inputs
  /// but different indices are considered redundant.
  ///
  /// @returns true if multiple definitions are found and replaced.
  ///
  /// @note This function does not recursively detect multiple definitions
  ///       due to replaced redundant arguments of gates.
  ///       The replaced gates are considered a new graph,
  ///       and this function must be called again
  ///       to verify that the new graph does not have multiple definitions.
  bool ProcessMultipleDefinitions() noexcept;

  /// Traverses the PDAG to collect multiple definitions of gates.
  ///
  /// @param[in] gate  The gate to traverse the sub-graph.
  /// @param[in,out] multi_def  Detected multiple definitions.
  /// @param[in,out] unique_gates  A set of semantically unique gates.
  ///
  /// @warning Gate marks must be clear.
  void DetectMultipleDefinitions(
      const GatePtr& gate,
      std::unordered_map<GatePtr, std::vector<GateWeakPtr>>* multi_def,
      GateSet* unique_gates) noexcept;

  /// Traverses the PDAG to detect modules.
  /// Modules are independent sub-graphs
  /// without common nodes with the rest of the graph.
  void DetectModules() noexcept;

  /// Traverses the given gate
  /// and assigns time of visit to nodes.
  ///
  /// @param[in] time  The current time.
  /// @param[in,out] gate  The gate to traverse and assign time to.
  ///
  /// @returns The final time of traversing.
  int AssignTiming(int time, const GatePtr& gate) noexcept;

  /// Determines modules from original gates
  /// that have been already timed.
  /// This function can also create new modules from the existing graph.
  ///
  /// @param[in,out] gate  The gate to test for modularity.
  void FindModules(const GatePtr& gate) noexcept;

  /// Processes gate arguments found during the module detection.
  ///
  /// @param[in,out] gate  The gate with the arguments.
  /// @param[in] non_shared_args  Args that belong only to this gate.
  /// @param[in,out] modular_args  Args that may be grouped into new modules.
  /// @param[in,out] non_modular_args  Args that cannot be grouped into modules.
  void ProcessModularArgs(
      const GatePtr& gate,
      const std::vector<std::pair<int, NodePtr>>& non_shared_args,
      std::vector<std::pair<int, NodePtr>>* modular_args,
      std::vector<std::pair<int, NodePtr>>* non_modular_args) noexcept;

  /// Creates a new module
  /// as an argument of an existing gate
  /// if the logic of the existing parent gate allows a sub-module.
  /// The existing arguments of the original gate
  /// are used to create the new module.
  /// If the new module must contain all the arguments,
  /// the original gate is asserted to be a module,
  /// and no operation is performed.
  ///
  /// @param[in,out] gate  The parent gate for a module.
  /// @param[in] args  Modular arguments to be added into the new module.
  ///
  /// @returns Pointer to the new module if it is created.
  GatePtr CreateNewModule(
      const GatePtr& gate,
      const std::vector<std::pair<int, NodePtr>>& args) noexcept;

  /// Checks if a group of modular arguments share
  /// anything with non-modular arguments.
  /// If so, then the modular arguments are not actually modular,
  /// and those arguments are removed from modular containers.
  /// This is due to chain of nodes
  /// that are shared between modular and non-modular arguments.
  ///
  /// @param[in,out] modular_args  Candidates for modular grouping.
  /// @param[in,out] non_modular_args  Non modular arguments.
  void FilterModularArgs(
      std::vector<std::pair<int, NodePtr>>* modular_args,
      std::vector<std::pair<int, NodePtr>>* non_modular_args) noexcept;

  /// Groups modular arguments by their common elements.
  /// The gates created with these modular arguments
  /// are guaranteed to be independent modules.
  ///
  /// @param[in,out] modular_args  Candidates for modular grouping.
  /// @param[out] groups  Grouped modular arguments.
  void GroupModularArgs(
      std::vector<std::pair<int, NodePtr>>* modular_args,
      std::vector<std::vector<std::pair<int, NodePtr>>>* groups) noexcept;

  /// Creates new module gates
  /// from groups of modular arguments
  /// if the logic of the parent gate allows sub-modules.
  /// The existing arguments of the original gate
  /// are used to create the new modules.
  /// If all the parent gate arguments are modular and within one group,
  /// the parent gate is asserted to be a module gate,
  /// and no operation is performed.
  ///
  /// @param[in,out] gate  The parent gate for a module.
  /// @param[in] modular_args  All the modular arguments.
  /// @param[in] groups  Grouped modular arguments.
  void CreateNewModules(
      const GatePtr& gate,
      const std::vector<std::pair<int, NodePtr>>& modular_args,
      const std::vector<std::vector<std::pair<int, NodePtr>>>& groups) noexcept;

  /// Gathers all modules in the PDAG.
  ///
  /// @returns Unique modules encountered breadth-first.
  ///
  /// @pre Module detection and marking has already been performed.
  ///
  /// @warning Gate marks are used.
  std::vector<GateWeakPtr> GatherModules() noexcept;

  /// Identifies common arguments of gates,
  /// and merges the common arguments into new gates.
  /// This technique helps uncover the common structure
  /// within gates that are not modules.
  ///
  /// @returns true if the graph structure is changed by this technique.
  ///
  /// @note This technique works only with OR/AND gates.
  ///       Partial or full normalization may make
  ///       this technique more effective.
  /// @note Constant arguments are not expected.
  ///
  /// @warning Gate marks are used for traversal.
  /// @warning Node counts are used for common node detection.
  bool MergeCommonArgs() noexcept;

  /// Merges common arguments for a specific group of gates.
  /// The gates are grouped by their operators.
  /// This is a helper function
  /// that divides the main merging technique by the gate types.
  ///
  /// @param[in] op  The operator that defines the group.
  ///
  /// @returns true if common args are merged into gates.
  ///
  /// @note The operator or logic of the gates must allow merging.
  ///       OR/AND operators are expected.
  ///
  /// @warning Gate marks are used for traversal.
  /// @warning Node counts are used for common node detection.
  bool MergeCommonArgs(Operator op) noexcept;

  /// Marks common arguments of gates with a specific operator.
  ///
  /// @param[in] gate  The gate to start the traversal.
  /// @param[in] op  The operator of gates
  ///                which arguments must be marked.
  ///
  /// @note Node count information is used to mark the common arguments.
  ///
  /// @warning Gate marks are used for linear traversal.
  void MarkCommonArgs(const GatePtr& gate, Operator op) noexcept;

  /// Helper struct for algorithms
  /// that must make an optimal decision
  /// how to merge or factor out
  /// common arguments of gates into new gates.
  struct MergeTable {
    using CommonArgs = std::vector<int>;  ///< Unique, sorted common arguments.
    using CommonParents = std::set<GatePtr>;  ///< Unique common parent gates.
    using Option = std::pair<CommonArgs, CommonParents>;  ///< One possibility.
    using OptionGroup = std::vector<Option*>;  ///< A set of best options.
    using MergeGroup = std::vector<Option>;  ///< Isolated group for processing.

    /// Candidate gates with their shared arguments.
    using Candidate = std::pair<GatePtr, CommonArgs>;
    /// Collection of merge-candidate gates with their common arguments.
    using Candidates = std::vector<Candidate>;
    /// Mapping for collection of common args and common parents as options.
    using Collection = boost::unordered_map<CommonArgs, CommonParents>;

    std::vector<MergeGroup> groups;  ///< Container of isolated groups.
  };

  /// Gathers common arguments of the gates
  /// in the group of a specific operator.
  /// The common arguments must be marked
  /// by the second visit exit time.
  ///
  /// @param[in] gate  The gate to start the traversal.
  /// @param[in] op  The operator of gates in the group.
  /// @param[out] group  The group of the gates with their common arguments.
  ///
  /// @note The common arguments are sorted.
  /// @note The gathering is limited by modules.
  ///       That is, no search is performed in submodules
  ///       because they don't have common args with the supermodule.
  ///
  /// @warning Gate marks are used for linear traversal.
  void GatherCommonArgs(const GatePtr& gate, Operator op,
                        MergeTable::Candidates* group) noexcept;

  /// Filters merge candidates and their shared arguments
  /// to detect opportunities for simplifications like gate substitutions.
  ///
  /// @param[in,out] candidates  The group of merge candidate gates
  ///
  /// @note The simplifications are based on optimistic heuristics,
  ///       and the end result may not be the most optimal.
  ///
  /// @warning May produce NULL type and constant gates.
  ///
  /// @todo Consider repeating until no change can be made.
  void FilterMergeCandidates(MergeTable::Candidates* candidates) noexcept;

  /// Groups candidates with common arguments.
  /// The groups do not intersect
  /// either by candidates or common arguments.
  ///
  /// @param[in,out] candidates  The group of the gates with their common args.
  /// @param[out] groups  Non-intersecting collection of groups of candidates.
  ///
  /// @note Groups with only one member are discarded.
  void GroupCandidatesByArgs(
      MergeTable::Candidates* candidates,
      std::vector<MergeTable::Candidates>* groups) noexcept;

  /// Finds intersections of common arguments of gates.
  /// Gates with the same common arguments are grouped
  /// to represent common parents for the arguments.
  ///
  /// @param[in] num_common_args  The least number common arguments to consider.
  /// @param[in] group  The group of the gates with their common arguments.
  /// @param[out] parents  Grouped common parent gates
  ///                      for the sets of common arguments.
  ///
  /// @note The common arguments are sorted.
  void GroupCommonParents(int num_common_args,
                          const MergeTable::Candidates& group,
                          MergeTable::Collection* parents) noexcept;

  /// Groups common args for merging.
  /// The common parents of arguments are isolated into groups
  /// so that other groups are not affected by the merging operations.
  ///
  /// @param[in] options  Combinations of common args and distributive gates.
  /// @param[out] table  Groups of distributive gates for separate manipulation.
  void GroupCommonArgs(const MergeTable::Collection& options,
                       MergeTable* table) noexcept;

  /// Finds an optimal way of grouping options.
  /// The goal of this function is
  /// to maximize the effect of gate merging
  /// or common argument factorization.
  ///
  /// After common arguments and parents are grouped,
  /// the merging technique must find the most optimal strategy
  /// to create new gates
  /// that will represent the common arguments.
  /// The strategy may favor modularity, size, or other parameters
  /// of the new structure of the final graph.
  /// The common elements within
  /// the groups of common parents and common arguments
  /// create the biggest challenge for finding the optimal solution.
  /// For example,
  /// {
  /// (a, b) : (p1, p2),
  /// (b, c) : (p2, p3)
  /// }
  /// The strategy has to make
  /// the most optimal choice
  /// between two mutually exclusive options.
  ///
  /// @param[in] all_options  The sorted set of options.
  ///                         The options must be sorted
  ///                         in ascending size of common arguments.
  /// @param[out] best_group  The optimal group of options.
  ///
  /// @note The all_options parameter is not passed by const reference
  ///       because the best group must store non const pointers to options.
  ///       It is expected that the chosen options will be manipulated
  ///       by the user of this function through these pointers.
  ///       However, this function guarantees
  ///       not to manipulate or change the set of all options.
  ///
  /// @todo The current logic misses opportunities
  ///       that may branch with the same base option.
  void FindOptionGroup(MergeTable::MergeGroup* all_options,
                       MergeTable::OptionGroup* best_group) noexcept;

  /// Finds the starting option for group formation.
  ///
  /// @param[in] all_options  The sorted set of options.
  ///                         The options must be sorted
  ///                         in ascending size of common arguments.
  /// @param[out] best_option  The optimal starting option if any.
  ///                          If not found, iterator at the end of the group.
  void FindBaseOption(MergeTable::MergeGroup* all_options,
                      MergeTable::MergeGroup::iterator* best_option) noexcept;

  /// Transforms common arguments of gates
  /// into new gates.
  ///
  /// @param[in,out] group  Group of merge options for manipulation.
  void TransformCommonArgs(MergeTable::MergeGroup* group) noexcept;

  /// Detects and manipulates AND and OR gate distributivity
  /// for the whole graph.
  ///
  /// @returns true if the graph is changed.
  bool DetectDistributivity() noexcept;

  /// Detects and manipulates AND and OR gate distributivity.
  /// For example,
  /// (a | b) & (a | c) = a | b & c.
  ///
  /// @param[in] gate  The gate which arguments and subgraph must be tested.
  ///
  /// @returns true if transformations are performed.
  ///
  /// @note This algorithm does not produce constant gates.
  /// @note NULL type gates are registered if produced.
  ///
  /// @warning Gate marks must be clear.
  bool DetectDistributivity(const GatePtr& gate) noexcept;

  /// Manipulates gates with distributive arguments.
  /// Designed to work with distributivity detection and manipulation logic.
  ///
  /// @param[in,out] gate  The gate which arguments must be manipulated.
  /// @param[in] distr_type  The type of distributive arguments.
  /// @param[in,out] candidates  Candidates for distributivity check.
  ///
  /// @returns true if transformations are performed.
  bool HandleDistributiveArgs(const GatePtr& gate,
                              Operator distr_type,
                              std::vector<GatePtr>* candidates) noexcept;

  /// Detects relationships between the gate and its distributive arguments
  /// to remove unnecessary candidates.
  /// The determination of redundant candidates follow the Boolean logic.
  /// For example, if any argument is superset of another argument,
  /// it can be removed from the gate.
  ///
  /// @param[in,out] gate  The gate which arguments must be filtered.
  /// @param[in,out] candidates  Candidates for distributivity check.
  ///
  /// @returns true if the candidates and the gate are manipulated.
  ///
  /// @note The redundant candidates are also removed from the gate.
  bool FilterDistributiveArgs(const GatePtr& gate,
                              std::vector<GatePtr>* candidates) noexcept;

  /// Groups distributive gate arguments
  /// for future factorization.
  /// The function tries to maximize the return
  /// from the gate manipulations.
  ///
  /// @param[in] options  Combinations of common args and distributive gates.
  /// @param[out] table  Groups of distributive gates for separate manipulation.
  ///
  /// @todo Evaluate various grouping strategies as in common arg merging.
  void GroupDistributiveArgs(const MergeTable::Collection& options,
                             MergeTable* table) noexcept;

  /// Transforms distributive of arguments gates
  /// into a new subgraph.
  ///
  /// @param[in,out] gate  The parent gate of all the distributive arguments.
  /// @param[in] distr_type  The type of distributive arguments.
  /// @param[in,out] group  Group of distributive args options for manipulation.
  void TransformDistributiveArgs(const GatePtr& gate, Operator distr_type,
                                 MergeTable::MergeGroup* group) noexcept;

  /// Propagates failures of common nodes to detect redundancy.
  /// The graph structure is optimized
  /// by removing the redundancies if possible.
  /// This optimization helps reduce the number of common nodes.
  ///
  /// @warning Boolean optimization may replace the root gate of the graph.
  /// @warning Node visit information is manipulated.
  /// @warning Gate marks are manipulated.
  /// @warning Node optimization values are manipulated.
  void BooleanOptimization() noexcept;

  /// Traverses the graph to find nodes
  /// that have more than one parent.
  /// Common nodes are encountered breadth-first,
  /// and they are unique.
  ///
  /// @param[out] common_gates  Gates with more than one parent.
  /// @param[out] common_variables  Common variables.
  ///
  /// @note Constant nodes are not expected to be operated.
  ///
  /// @warning Node visit information is manipulated.
  void GatherCommonNodes(
      std::vector<GateWeakPtr>* common_gates,
      std::vector<std::weak_ptr<Variable>>* common_variables) noexcept;

  /// Tries to simplify the graph by removing redundancies
  /// generated by a common node.
  ///
  /// @tparam N  Non-Node, concrete (i.e. Gate, etc.) type.
  ///
  /// @param[in] common_node  A node with more than one parent.
  template <class N>
  void ProcessCommonNode(const std::weak_ptr<N>& common_node) noexcept;

  /// Marks ancestor gates true.
  /// The marking stops at the root
  /// of an independent subgraph for algorithmic efficiency.
  ///
  /// @param[in] node  The child node.
  /// @param[out] module  The root module gate ancestor.
  ///
  /// @pre Gate marks are clear.
  ///
  /// @warning Since very specific branches are marked 'true',
  ///          cleanup must be performed after/with the use of the ancestors.
  ///          If the cleanup is done improperly or not at all,
  ///          the default global contract of clean marks will be broken.
  void MarkAncestors(const NodePtr& node, GatePtr* module) noexcept;

  /// Propagates failure or success of a common node
  /// by setting its ancestors' optimization values to 1 or -1
  /// if they fail or succeed according to their Boolean logic.
  /// The failure of an argument is similar to propagating constant TRUE/1.
  /// The success of an argument is similar to propagating constant FALSE/-1.
  ///
  /// @param[in,out] gate  The root gate under consideration.
  /// @param[in] node  The node that is the source of failure.
  ///
  /// @returns Total multiplicity of the node.
  ///
  /// @pre The optimization value of the main common node is 1 or -1.
  /// @pre The marks of ancestor gates are 'true'.
  ///
  /// @post The marks of all ancestor gates are reset to 'false'.
  /// @post All ancestor gates are marked with the descendant index.
  int PropagateState(const GatePtr& gate, const NodePtr& node) noexcept;

  /// Determines if a gate fails or succeeds
  /// due to failed/succeeded arguments.
  /// If gates fails, its optimization value is set to 1.
  /// If it succeeds, its optimization value is -1.
  /// If the state is indeterminate, the optimization value is 0.
  ///
  /// @param[in,out] gate  The gate with the arguments.
  /// @param[in] num_failure  The number of failure (TRUE/1) arguments.
  /// @param[in] num_success  The number of success (FALSE/-1) arguments.
  void DetermineGateState(const GatePtr& gate, int num_failure,
                          int num_success) noexcept;

  /// Collects failure or success destinations
  /// and marks non-redundant nodes.
  /// The optimization value for non-redundant gates are set to 2.
  ///
  /// @param[in] gate  The non-failed gate which sub-graph is to be traversed.
  /// @param[in] index  The index of the main state-source common node.
  /// @param[in,out] destinations  Destinations of the state.
  ///
  /// @returns The number of encounters with the destinations.
  int CollectStateDestinations(
      const GatePtr& gate,
      int index,
      std::unordered_map<int, GateWeakPtr>* destinations) noexcept;

  /// Detects if parents of a node are redundant.
  ///
  /// @param[in] node  The common node.
  /// @param[in,out] destinations  Destinations of the state.
  /// @param[out] redundant_parents  A set of redundant parents.
  ///
  /// @pre Destinations have valid state (Success or Failure).
  /// @pre Non-redundant parents have optimization value 2.
  ///
  /// @post Destinations that are also redundant parents are removed
  ///       if the semantics of the transformations is equivalent.
  void CollectRedundantParents(
      const NodePtr& node,
      std::unordered_map<int, GateWeakPtr>* destinations,
      std::vector<GateWeakPtr>* redundant_parents) noexcept;

  /// Detects if parents of a node are redundant.
  /// If there are redundant parents,
  /// depending on the logic of the parent,
  /// the node is removed from the parent
  /// unless it is also in the destination set with specific logic.
  /// In the latter case, the parent is removed from the destinations.
  ///
  /// @param[in] node  The common node.
  /// @param[in] redundant_parents  A set of redundant parents.
  ///
  /// @note Constant gates are registered for removal.
  /// @note Null type gates are registered for removal.
  void ProcessRedundantParents(
      const NodePtr& node,
      const std::vector<GateWeakPtr>& redundant_parents) noexcept;

  /// Transforms failure or success destination
  /// according to the logic and the common node.
  ///
  /// @tparam N  Non-Node, concrete (i.e. Gate, etc.) type.
  ///
  /// @param[in] node  The common node.
  /// @param[in] destinations  Destination gates for the state.
  ///
  /// @warning This function will replace the root gate of the graph
  ///          if it is the destination.
  template <class N>
  void ProcessStateDestinations(
      const std::shared_ptr<N>& node,
      const std::unordered_map<int, GateWeakPtr>& destinations) noexcept;

  /// Clears all the ancestor marks used in Boolean optimization steps.
  ///
  /// @param[in] gate  The top ancestor of the common node.
  ///
  /// @pre All ancestor gates are marked with the descendant index.
  /// @pre The common node itself is not the ancestor.
  ///
  /// @warning The common node must be cleaned separately.
  void ClearStateMarks(const GatePtr& gate) noexcept;

  /// The Shannon decomposition for common nodes in the PDAG.
  /// This procedure is also called "Constant Propagation",
  /// but it is confusing with the actual propagation of
  /// house events and constant gates.
  ///
  /// The main two operations are performed
  /// according to the Shannon decomposition of particular setups:
  ///
  /// x & f(x, y) = x & f(1, y)
  /// x | f(x, y) = x | f(0, y)
  ///
  /// @returns true if the setups are found and processed.
  ///
  /// @note This preprocessing algorithm
  ///       may introduce clones of existing shared gates,
  ///       which may increase the size of the graph
  ///       and complicate application and performance of other algorithms.
  ///
  /// @warning Gate descendant marks are used.
  /// @warning Gate ancestor marks are used.
  /// @warning Node visit information is used.
  /// @warning Gate marks are used.
  bool DecomposeCommonNodes() noexcept;

  /// Functor for processing of decomposition setups with common nodes.
  class DecompositionProcessor {
   public:
    /// Launches the processing of decomposition setups in preprocessing.
    /// Processes common nodes in decomposition setups.
    /// This function only works with DecomposeCommonNodes()
    /// because it requires a specific setup of
    /// descendant marks and visit information of nodes.
    /// These setups are assumed
    /// to be provided by the DecomposeCommonNodes().
    ///
    /// @param[in,out] common_node  Common node to be processed.
    /// @param[in,out] preprocessor  The host preprocessor with state.
    ///
    /// @returns true if the decomposition setups are found and processed.
    bool operator()(const std::weak_ptr<Node>& common_node,
                    Preprocessor* preprocessor) noexcept;

   private:
    /// Marks destinations for common node decomposition.
    /// The descendant mark of some ancestors of the common node
    /// is set to the index of the common node.
    /// Parents of the common node may not get marked
    /// unless they are parents of parents.
    ///
    /// @param[in] parent  The parent or ancestor of the common node.
    ///
    /// @pre No ancestor gate has 'dirty' descendant marks with the index
    ///      before the call of this function.
    ///      Otherwise, the logic of the algorithm is messed up and invalid.
    /// @pre Marking is limited by a single root module.
    ///
    /// @post The ancestor gate descendant marks are set to the index.
    void MarkDestinations(const GatePtr& parent) noexcept;

    /// Processes decomposition destinations
    /// with the decomposition setups.
    ///
    /// @param[in] dest  The set of destination parents.
    ///
    /// @returns true if the graph is changed by processing.
    ///
    /// @warning Gate marks are used to traverse subgraphs in linear time.
    /// @warning Gate descendant marks are used to detect ancestor.
    /// @warning Gate ancestor marks are used to detect sub-graphs.
    /// @warning Gate visit time information is used to detect shared nodes.
    bool ProcessDestinations(const std::vector<GateWeakPtr>& dest) noexcept;

    /// Processes decomposition ancestors
    /// in the link to the decomposition destinations.
    /// Common node's parents shared outside of the subgraph
    /// may get cloned not to mess the whole graph.
    ///
    /// @param[in] ancestor  The parent or ancestor of the common node.
    /// @param[in] state  The constant state to be propagated.
    /// @param[in] root  The root of the graph,
    ///                  the decomposition destination.
    ///
    /// @returns true if the parent is reached and processed.
    ///
    /// @warning Gate marks are used to traverse subgraphs in linear time.
    ///          Gate marks must be clear for the subgraph for the first call.
    /// @warning Gate descendant marks are used to detect ancestors.
    /// @warning Gate ancestor marks are used to detect sub-graphs.
    /// @warning Gate visit time information is used to detect shared nodes.
    bool ProcessAncestors(const GatePtr& ancestor, bool state,
                          const GatePtr& root) noexcept;

    /// Determines if none of the gate ancestors of the node
    /// is outside of the given graph.
    ///
    /// @param[in] gate  The starting ancestor.
    /// @param[in] root  The root of the graph.
    ///
    /// @returns true if all the ancestors are within the graph.
    ///
    /// @pre Already processed ancestors are marked with the root index.
    ///      The positive ancestor mark means the result is true,
    ///      the negative mark means the ancestry is outside of the graph.
    ///
    /// @post The ancestor gates are marked with the signed root gate index
    ///       as explained in the precondition.
    static bool IsAncestryWithinGraph(const GatePtr& gate,
                                      const GatePtr& root) noexcept;

    /// Clears only the used ancestor marks.
    ///
    /// @param[in] gate  The starting ancestor
    ///                  that is likely to be marked.
    /// @param[in] root  The root of the graph.
    ///
    /// @post Gate ancestor marks are set to 0.
    static void ClearAncestorMarks(const GatePtr& gate,
                                   const GatePtr& root) noexcept;

    NodePtr node_;  ///< The common node to process.
    Preprocessor* preprocessor_ = nullptr;  ///< The host preprocessor.
  };

  /// Replaces one gate in the graph with another.
  ///
  /// @param[in,out] gate  An existing gate to be replaced.
  /// @param[in,out] replacement  A gate that will replace the old gate.
  ///
  /// @post The sign of the existing gate as an argument
  ///       is transferred to the replacement gate.
  ///
  /// @post If any parent becomes constant or NULL type,
  ///       the parent is registered for removal.
  void ReplaceGate(const GatePtr& gate, const GatePtr& replacement) noexcept;

  /// Registers mutated gates for potential deletion later.
  ///
  /// @param[in] gate  The mutated gate under examination.
  ///
  /// @returns true if the gate is registered for clearance.
  ///
  /// @pre The caller will later call the appropriate cleanup functions.
  bool RegisterToClear(const GatePtr& gate) noexcept;

  /// Gathers all nodes in the PDAG.
  ///
  /// @param[out] gates  A set of gates.
  /// @param[out] variables  A set of variables.
  ///
  /// @warning Node visit information is manipulated.
  void GatherNodes(std::vector<GatePtr>* gates,
                   std::vector<VariablePtr>* variables) noexcept;

  /// Gathers nodes in the sub-graph.
  ///
  /// @param[in] gate  The root gate to traverse.
  /// @param[in,out] gates  A set of gates.
  /// @param[in,out] variables  A set of variables.
  ///
  /// @pre Visited nodes are marked.
  void GatherNodes(const GatePtr& gate, std::vector<GatePtr>* gates,
                   std::vector<VariablePtr>* variables) noexcept;

  /// @todo Eliminate the protected data.
  Pdag* graph_;  ///< The PDAG to preprocess.
};

/// Undefined template class for specialization of Preprocessor
/// for needs of specific analysis algorithms.
///
/// @tparam Algorithm  The target algorithm for the preprocessor.
template <class Algorithm>
class CustomPreprocessor;

class Bdd;

/// Specialization of preprocessing for BDD based analyses.
template <>
class CustomPreprocessor<Bdd> : public Preprocessor {
 public:
  using Preprocessor::Preprocessor;

 private:
  /// Performs preprocessing for analyses with Binary Decision Diagrams.
  /// This preprocessing assigns the order for variables for BDD construction.
  void Run() noexcept override;
};

class Zbdd;

/// Specialization of preprocessing for ZBDD based analyses.
template <>
class CustomPreprocessor<Zbdd> : public Preprocessor {
 public:
  using Preprocessor::Preprocessor;

 protected:
  /// Performs preprocessing for analyses
  /// with Zero-Suppressed Binary Decision Diagrams.
  /// Complements are propagated to variables.
  /// This preprocessing assigns the order for variables for ZBDD construction.
  void Run() noexcept override;
};

class Mocus;

/// Specialization of preprocessing for MOCUS based analyses.
template <>
class CustomPreprocessor<Mocus> : public CustomPreprocessor<Zbdd> {
 public:
  using CustomPreprocessor<Zbdd>::CustomPreprocessor;

 private:
  /// Performs processing of a fault tree
  /// to simplify the structure to
  /// normalized (OR/AND gates only),
  /// modular (independent sub-trees),
  /// positive-gate-only (negation normal) PDAG.
  /// The variable ordering is assigned specifically for MOCUS.
  void Run() noexcept override;

  /// Groups and inverts the topological ordering for nodes.
  /// The inversion is done to simplify the work of MOCUS facilities,
  /// which rely on the top-down approach.
  ///
  /// Gates are ordered so that they show up at the top of ZBDD.
  /// However, module gates are handled just as basic events.
  /// Basic events preserve
  /// the original topological ordering.
  ///
  /// Note, however, the inversion of the order
  /// generally (dramatically) increases the size of Binary Decision Diagrams.
  void InvertOrder() noexcept;
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_PREPROCESSOR_H_
