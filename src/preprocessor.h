/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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
/// A collection of Boolean graph preprocessing algorithms
/// that simplify fault trees for analysis.

#ifndef SCRAM_SRC_PREPROCESSOR_H_
#define SCRAM_SRC_PREPROCESSOR_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/unordered_map.hpp>

#include "boolean_graph.h"

namespace scram {

namespace test { class PreprocessorTest; }

/// @class Preprocessor
/// The class provides main preprocessing operations
/// over a Boolean graph
/// to simplify the fault tree
/// and to help generate minimal cut sets more efficiently.
class Preprocessor {
  friend class test::PreprocessorTest;

 public:
  /// Constructs a preprocessor of a Boolean graph
  /// representing a fault tree.
  ///
  /// @param[in] graph The Boolean graph to be preprocessed.
  ///
  /// @warning There should not be another shared pointer to the root gate
  ///          outside of the passed Boolean graph.
  ///          Upon preprocessing a new root gate may be assigned to the graph,
  ///          and, if there is an extra pointer to the previous top gate
  ///          outside of the graph,
  ///          the destructor will not be called
  ///          as expected by the preprocessing algorithms,
  ///          which will mess the new structure of the Boolean graph.
  explicit Preprocessor(BooleanGraph* graph) noexcept;

  /// Performs processing of a fault tree
  /// to simplify the structure to
  /// normalized (OR/AND gates only),
  /// modular (independent sub-trees),
  /// positive-gate-only (negation normal)
  /// indexed fault tree.
  ///
  /// @warning There should not be another smart pointer
  ///          to the indexed top gate of the fault tree
  ///          outside of the Boolean graph.
  void ProcessFaultTree() noexcept;

 private:
  typedef std::shared_ptr<Node> NodePtr;
  typedef std::shared_ptr<IGate> IGatePtr;
  typedef std::weak_ptr<IGate> IGateWeakPtr;
  typedef std::shared_ptr<Variable> VariablePtr;
  typedef std::shared_ptr<Constant> ConstantPtr;

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
  void PhaseOne() noexcept;

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
  void PhaseTwo() noexcept;

  /// Application of gate normalization.
  /// After this phase,
  /// the graph is in normal form.
  ///
  /// @note Gate normalization is conducted.
  void PhaseThree() noexcept;

  /// Propagation of complements.
  /// Complements are propagated down to the variables in the graph.
  /// After this phase,
  /// the graph is in negation normal form.
  ///
  /// @note Complements are propagated to the variables of the graph.
  void PhaseFour() noexcept;

  /// The final phase
  /// that cleans up the graph,
  /// and puts the structure of the graph ready for analysis.
  /// This phase makes the graph structure
  /// alternating AND/OR gate layers.
  void PhaseFive() noexcept;

  /// Checks the root gate of the graph for further processing.
  /// The root gate may become constant
  /// or one-variable-NULL-gate,
  /// which signals the special case
  /// and no need for further processing.
  ///
  /// @returns true if no more processing is needed.
  ///
  /// @note This function may swap the root gate of the graph.
  bool CheckRootGate() noexcept;

  /// Removes argument gates of NULL type,
  /// which means these arg gates have only one argument.
  /// That one grand arg is transfered to the parent gate,
  /// and the original argument gate is removed from the parent gate.
  ///
  /// This function is used only once
  /// to get rid of all NULL type gates
  /// at the very beginning of preprocessing.
  ///
  /// @note This function assumes
  ///       that the container for NULL gates is empty.
  ///       In other words, it is assumed
  ///       no other function was trying to
  ///       communicate NULL type gates for future processing.
  /// @note This function is designed to be called only once
  ///       at the start of preprocessing
  ///       after cleaning all the constants from the graph.
  ///
  /// @warning There still may be only one NULL type gate
  ///          which is the root of the graph.
  ///          This must be handled separately.
  /// @warning NULL gates that are constant are not handled
  ///          and left for constant propagation functions.
  void RemoveNullGates() noexcept;

  /// Removes all Boolean constants from the Boolean graph
  /// according to the Boolean logic of the gates.
  /// This function is only used
  /// to get rid of all constants
  /// registered by the Boolean graph
  /// at the very beginning of preprocessing.
  ///
  /// @note This is one of the first preprocessing steps.
  ///       Other algorithms are safe to assume
  ///       that there are no house events in the fault tree.
  ///       Only possible constant nodes are gates
  ///       that turn NULL or UNITY sets.
  ///
  /// @warning There still may be only one constant state gate
  ///          which is the root of the graph.
  ///          This must be handled separately.
  void RemoveConstants() noexcept;

  /// Propagates a Boolean constant bottom-up.
  /// This is a helper function for initial cleanup of the Boolean graph.
  ///
  /// @param[in,out] constant The constant to be propagated.
  ///
  /// @note This function works together with
  ///       NULL type and constant gate propagation functions
  ///       to clean the results of the propagation.
  void PropagateConstant(const ConstantPtr& constant) noexcept;

  /// Changes the state of a gate
  /// or removes a constant argument.
  /// The function determines its actions depending on
  /// the type of a gate and state of an argument.
  ///
  /// @param[in,out] gate The parent gate that contains the arguments.
  /// @param[in] arg The positive or negative index of the argument.
  /// @param[in] state False or True constant state of the argument.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function takes into account the sign of the index
  ///       to properly assess the Boolean constant argument.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  void ProcessConstantArg(const IGatePtr& gate, int arg, bool state) noexcept;

  /// Processes Boolean constant argument with True value.
  ///
  /// @param[in,out] gate The parent gate that contains the arguments.
  /// @param[in] arg The positive or negative index of the argument.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  void ProcessTrueArg(const IGatePtr& gate, int arg) noexcept;

  /// Processes Boolean constant argument with False value.
  ///
  /// @param[in,out] gate The parent gate that contains the arguments.
  /// @param[in] arg The positive or negative index of the argument.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  void ProcessFalseArg(const IGatePtr& gate, int arg) noexcept;

  /// Removes Boolean constant arguments from a gate
  /// taking into account the logic.
  /// This is a helper function
  /// for NULL and UNITY set or constant propagation for the graph.
  /// If the final gate is empty,
  /// its state is turned into NULL or UNITY
  /// depending on the logic of the gate
  /// and the logic of the Boolean constant propagation.
  ///
  /// @param[in,out] gate The gate that contains the arguments to be removed.
  /// @param[in] arg The positive or negative index of the argument.
  ///
  /// @note This is a helper function that propagates constants,
  ///       so it is coupled with the logic of
  ///       the constant propagation algorithms.
  ///
  /// @warning This function does not handle complex K/N gate parents.
  ///          The logic is not simple for K/N gates,
  ///          so it must be handled by the caller.
  void RemoveConstantArg(const IGatePtr& gate, int arg) noexcept;

  /// Propagates constant gates bottom-up.
  /// This is a helper function for algorithms
  /// that may produce and need to remove constant gates.
  ///
  /// @param[in,out] gate The gate that has become constant.
  ///
  /// @note This function works together with
  ///       NULL type gate propagation function
  ///       to cleanup the structure of the graph.
  ///
  /// @warning All parents of the gate will be removed,
  ///          so the gate itself may get deleted
  ///          unless it is the top gate.
  void PropagateConstGate(const IGatePtr& gate) noexcept;

  /// Propagate NULL type gates bottom-up.
  /// This is a helper function for algorithms
  /// that may produce and need to remove NULL type gates.
  ///
  /// @param[in,out] gate The gate that is NULL type.
  ///
  /// @note This function works together with
  ///       constant state gate propagation function
  ///       to cleanup the structure of the graph.
  ///
  /// @warning All parents of the gate will be removed,
  ///          so the gate itself may get deleted
  ///          unless it is the top gate.
  void PropagateNullGate(const IGatePtr& gate) noexcept;

  /// Clears all constant gates registered for removal
  /// by algorithms or other preprocessing functions.
  ///
  /// @warning Gate marks will get cleared by this function.
  void ClearConstGates() noexcept;

  /// Clears all NULL type gates registered for removal
  /// by algorithms or other preprocessing functions.
  ///
  /// @warning Gate marks will get cleared by this function.
  void ClearNullGates() noexcept;

  /// Normalizes the gates of the whole Boolean graph
  /// into OR, AND gates.
  ///
  /// @param[in] full A flag to handle complex gates like XOR and K/N,
  ///                 which generate a lot more new gates
  ///                 and make the structure of the graph more complex.
  ///
  /// @note The negation of the top gate is saved
  ///       and handled as a special case for negation propagation
  ///       because it does not have a parent.
  /// @note New gates are created only upon full normalization
  ///       of complex gates like XOR and K/N.
  /// @note The full normalization is meant to be called only once.
  ///
  /// @warning The root get may still be NULL type.
  void NormalizeGates(bool full) noexcept;

  /// Notifies all parents of negative gates,
  /// such as NOT, NOR, and NAND,
  /// before transforming these gates
  /// into basic gates of OR and AND.
  /// The argument gates are swapped with a negative sign.
  ///
  /// @param[in] gate The gate to start processing.
  ///
  /// @note This function is a helper function for NormalizeGates().
  ///
  /// @warning Gate marks must be clear.
  /// @warning This function does not change the types of gates.
  /// @warning The root gate does not have parents,
  ///          so it is not handled here.
  void NotifyParentsOfNegativeGates(const IGatePtr& gate) noexcept;

  /// Normalizes complex gates into OR, AND gates.
  ///
  /// @param[in,out] gate The gate to be processed.
  /// @param[in] full A flag to handle complex gates like XOR and K/N.
  ///
  /// @note This is a helper function for NormalizeGates().
  ///
  /// @note This function registers NULL type gates for future removal.
  ///
  /// @warning Gate marks must be clear.
  /// @warning The parents of negative gates are assumed to be
  ///          notified about the change of their arguments' types.
  void NormalizeGate(const IGatePtr& gate, bool full) noexcept;

  /// Normalizes a gate with XOR logic.
  /// This is a helper function
  /// for the main gate normalization function.
  ///
  /// @param[in,out] gate The gate to normalize.
  ///
  /// @note This is a helper function for NormalizeGate.
  void NormalizeXorGate(const IGatePtr& gate) noexcept;

  /// Normalizes an ATLEAST gate with a vote number.
  /// The gate is turned into an OR gate of
  /// recursively normalized ATLEAST and AND arg gates
  /// according to the formula
  /// K/N(x, y_i) = OR(AND(x, K-1/N-1(y_i)), K/N-1(y_i)))
  /// with y_i being the rest of formula arguments,
  /// which exclude x.
  /// This representation is more friendly
  /// to other preprocessing and analysis techniques
  /// than the alternative,
  /// which is OR of AND gates of combinations.
  ///
  /// @param[in,out] gate The ATLEAST gate to normalize.
  ///
  /// @note This is a helper function for NormalizeGate.
  void NormalizeAtleastGate(const IGatePtr& gate) noexcept;

  /// Propagates complements of argument gates down to leafs
  /// according to the De Morgan's law
  /// in order to remove any negative logic from the graph's gates.
  /// The resulting graph will contain only positive gates, OR and AND types.
  /// After this function, the Boolean graph is in negation normal form.
  ///
  /// @param[in,out] gate The starting gate to traverse the graph.
  ///                     This is for recursive purposes.
  /// @param[in,out] gate_complements The processed complements of gates.
  ///
  /// @note The graph must be normalized.
  ///       It must contain only OR and AND gates.
  ///
  /// @warning Gate marks must be clear.
  /// @warning If the root gate has a negative sign,
  ///          it must be handled before calling this function.
  ///          The arguments and type of the gate
  ///          must be inverted according to the logic of the root gate.
  ///
  /// @todo Module-aware complement propagation.
  void PropagateComplements(const IGatePtr& gate,
                            std::map<int, IGatePtr>* gate_complements) noexcept;

  /// Coalesces positive argument gates
  /// with the same OR or AND logic as parents.
  /// This function merges similar logic gates of NAND and NOR as well.
  ///
  /// @param[in,out] gate The starting gate to traverse the graph.
  ///                     This is for recursive purposes.
  /// @param[in] common A flag to also join common gates.
  ///                   These gates may be important for other algorithms.
  ///
  /// @returns true if the given graph has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @note Constant state gates may be generated upon joining.
  ///       These gates are registered for future processing.
  /// @note It is impossible that this function generates NULL type gates.
  /// @note Module gates are omitted from coalescing to preserve them.
  bool JoinGates(const IGatePtr& gate, bool common) noexcept;

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

  /// Traverses the Boolean graph to collect multiple definitions of gates.
  ///
  /// @param[in] gate The gate to traverse the sub-graph.
  /// @param[in,out] multi_def Detected multiple definitions.
  /// @param[in,out] unique_gates A set of semantically unique gates.
  ///
  /// @warning Gate marks must be clear.
  void DetectMultipleDefinitions(
      const IGatePtr& gate,
      std::unordered_map<IGatePtr, std::vector<IGateWeakPtr> >* multi_def,
      GateSet* unique_gates) noexcept;

  /// Traverses the Boolean graph to detect modules.
  /// Modules are independent sub-graphs
  /// without common nodes with the rest of the graph.
  void DetectModules() noexcept;

  /// Traverses the given gate
  /// and assigns time of visit to nodes.
  ///
  /// @param[in] time The current time.
  /// @param[in,out] gate The gate to traverse and assign time to.
  ///
  /// @returns The final time of traversing.
  int AssignTiming(int time, const IGatePtr& gate) noexcept;

  /// Determines modules from original gates
  /// that have been already timed.
  /// This function can also create new modules from the existing graph.
  ///
  /// @param[in,out] gate The gate to test for modularity.
  ///
  /// @todo Make this function aware of previously created modules.
  void FindModules(const IGatePtr& gate) noexcept;

  /// Processes gate arguments found during the module detection.
  ///
  /// @param[in,out] gate The gate with the arguments.
  /// @param[in] non_shared_args Args that belong only to this gate.
  /// @param[in,out] modular_args Args that may be grouped into new modules.
  /// @param[in,out] non_modular_args Args that cannot be grouped into modules.
  void ProcessModularArgs(
      const IGatePtr& gate,
      const std::vector<std::pair<int, NodePtr> >& non_shared_args,
      std::vector<std::pair<int, NodePtr> >* modular_args,
      std::vector<std::pair<int, NodePtr> >* non_modular_args) noexcept;

  /// Creates a new module
  /// as an argument of an existing gate
  /// if the logic of the existing parent gate allows a sub-module.
  /// The existing arguments of the original gate
  /// are used to create the new module.
  /// If the new module must contain all the arguments,
  /// the original gate is asserted to be a module,
  /// and no operation is performed.
  ///
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] args Modular arguments to be added into the new module.
  ///
  /// @returns Pointer to the new module if it is created.
  IGatePtr CreateNewModule(
      const IGatePtr& gate,
      const std::vector<std::pair<int, NodePtr> >& args) noexcept;

  /// Checks if a group of modular arguments share
  /// anything with non-modular arguments.
  /// If so, then the modular arguments are not actually modular,
  /// and those arguments are removed from modular containers.
  /// This is due to chain of nodes
  /// that are shared between modular and non-modular arguments.
  ///
  /// @param[in,out] modular_args Candidates for modular grouping.
  /// @param[in,out] non_modular_args Non modular arguments.
  void FilterModularArgs(
      std::vector<std::pair<int, NodePtr> >* modular_args,
      std::vector<std::pair<int, NodePtr> >* non_modular_args) noexcept;

  /// Groups modular arguments by their common elements.
  /// The gates created with these modular arguments
  /// are guaranteed to be independent modules.
  ///
  /// @param[in] modular_args Candidates for modular grouping.
  /// @param[out] groups Grouped modular arguments.
  void GroupModularArgs(
      const std::vector<std::pair<int, NodePtr> >& modular_args,
      std::vector<std::vector<std::pair<int, NodePtr> > >* groups) noexcept;

  /// Creates new module gates
  /// from groups of modular arguments
  /// if the logic of the parent gate allows sub-modules.
  /// The existing arguments of the original gate
  /// are used to create the new modules.
  /// If all the parent gate arguments are modular and within one group,
  /// the parent gate is asserted to be a module gate,
  /// and no operation is performed.
  ///
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] modular_args All the modular arguments.
  /// @param[in] groups Grouped modular arguments.
  void CreateNewModules(
      const IGatePtr& gate,
      const std::vector<std::pair<int, NodePtr>>& modular_args,
      const std::vector<std::vector<std::pair<int, NodePtr>>>& groups) noexcept;

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
  /// @param[in] op The operator that defines the group.
  ///
  /// @returns true if common args are merged into gates.
  ///
  /// @note The operator or logic of the gates must allow merging.
  ///       OR/AND operators are expected.
  ///
  /// @warning Gate marks are used for traversal.
  /// @warning Node counts are used for common node detection.
  bool MergeCommonArgs(const Operator& op) noexcept;

  /// Marks common arguments of gates with a specific operator.
  ///
  /// @param[in] gate The gate to start the traversal.
  /// @param[in] op The operator of gates
  ///               which arguments must be marked.
  ///
  /// @note Node count information is used to mark the common arguments.
  ///
  /// @warning Gate marks are used for linear traversal.
  void MarkCommonArgs(const IGatePtr& gate, const Operator& op) noexcept;

  /// @struct MergeTable
  /// Helper struct for algorithms
  /// that must make an optimal decision
  /// how to merge or factor out
  /// common arguments of gates into new gates.
  struct MergeTable {
    typedef std::vector<int> CommonArgs;  ///< Unique, sorted common arguments.
    typedef std::set<IGatePtr> CommonParents;  ///< Unique common parent gates.
    typedef std::pair<CommonArgs, CommonParents> Option;  ///< One possibility.
    typedef std::vector<Option*> OptionGroup;  ///< A set of best options.
    typedef std::vector<Option> MergeGroup;  ///< Isolated group for processing.

    /// Collection of merge-candidate gates with their common arguments.
    typedef std::vector<std::pair<IGatePtr, CommonArgs>> Candidates;
    /// Mapping for collection of common args and common parents as options.
    typedef boost::unordered_map<CommonArgs, CommonParents> Collection;

    std::vector<MergeGroup> groups;  ///< Container of isolated groups.
  };

  /// Gathers common arguments of the gates
  /// in the group of a specific operator.
  /// The common arguments must be marked
  /// by the second visit exit time.
  ///
  /// @param[in] gate The gate to start the traversal.
  /// @param[in] op The operator of gates in the group.
  /// @param[out] group The group of the gates with their common arguments.
  ///
  /// @note The common arguments are sorted.
  ///
  /// @warning Gate marks are used for linear traversal.
  void GatherCommonArgs(const IGatePtr& gate, const Operator& op,
                        MergeTable::Candidates* group) noexcept;

  /// Findes intersections of common arguments of gates.
  /// Gates with the same common arguments are grouped
  /// to represent common parents for the arguments.
  ///
  /// @param[in] num_common_args The least number common arguments to consider.
  /// @param[in] group The group of the gates with their common arguments.
  /// @param[out] parents Grouped common parent gates
  ///             for the sets of common arguments.
  ///
  /// @note The common arguments are sorted.
  void GroupCommonParents(int num_common_args,
                          const MergeTable::Candidates& group,
                          MergeTable::Collection* parents) noexcept;

  /// Groups common args for merging.
  /// The common parents of arguments are isolated into groups
  /// so that other groups are not affected by the merging operations.
  ///
  /// @param[in] options Combinations of common args and distributive gates.
  /// @param[out] table Groups of distributive gates for separate manipulation.
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
  /// @param[in] all_options The sorted set of options.
  ///                        The options are sorted
  ///                        in descending size of common arguments.
  /// @param[out] best_group The optimal group of options.
  ///
  /// @note The all_options parameter is not passed by const reference
  ///       because the best group must store non const pointers to options.
  ///       It is expected that the chosen options will be manipulated
  ///       by the user of this function through these pointers.
  ///       However, this function guarantees
  ///       not to manipulate or change the set of all options.
  ///
  /// @todo The non const reference seems to be a code-smell in this case.
  /// @todo Evaluate various merging strategies.
  ///       Module creation, the number of parents,
  ///       the number of merge groups, and other criteria
  ///       can serve as optimization parameters.
  /// @todo The current logic misses opportunities
  ///       that may branch with the same base option.
  void FindOptionGroup(MergeTable::MergeGroup& all_options,
                       MergeTable::OptionGroup* best_group) noexcept;

  /// Transforms common arguments of gates
  /// into new gates.
  ///
  /// @param[in,out] group Group of merge options for manipulation.
  void TransformCommonArgs(MergeTable::MergeGroup* group) noexcept;

  /// Detects and manipulates AND and OR gate distributivity.
  /// For example,
  /// (a | b) & (a | c) = a | b & c.
  ///
  /// @param[in] gate The gate which arguments must be tested.
  ///
  /// @returns true if transformations are performed.
  ///
  /// @warning Gate marks must be clear.
  bool DetectDistributivity(const IGatePtr& gate) noexcept;

  /// Manipulates gates with distributive arguments.
  ///
  /// @param[in,out] gate The gate which arguments must be manipulated.
  /// @param[in] distr_type The type of distributive arguments.
  /// @param[in,out] candidates Candidates for distributivity check.
  ///
  /// @returns true if transformations are performed.
  bool HandleDistributiveArgs(const IGatePtr& gate,
                              const Operator& distr_type,
                              const std::vector<IGatePtr>& candidates) noexcept;

  /// Groups distributive gate arguments
  /// for furture factorization.
  /// The function tries to maximize the return
  /// from the gate manipulations.
  ///
  /// @param[in] options Combinations of common args and distributive gates.
  /// @param[out] table Groups of distributive gates for separate manipulation.
  ///
  /// @todo Evaluate various grouping strategies as in common arg merging.
  void GroupDistributiveArgs(const MergeTable::Collection& options,
                             MergeTable* table) noexcept;

  /// Transforms distributive of arguments gates
  /// into a new subgraph.
  ///
  /// @param[in,out] gate The parent gate of all the distributive arguments.
  /// @param[in] distr_type The type of distributive arguments.
  /// @param[in,out] group Group of distributive args options for manipulation.
  void TransformDistributiveArgs(const IGatePtr& gate,
                                 const Operator& distr_type,
                                 MergeTable::MergeGroup* group) noexcept;

  /// Propagates failures of common nodes to detect redundancy.
  /// The graph structure is optimized
  /// by removing the redundancies if possible.
  /// This optimization helps reduce the number of common nodes.
  ///
  /// @warning Boolean optimization may replace the root gate of the graph.
  /// @warning The current implementation works
  ///          only for coherent graphs.
  void BooleanOptimization() noexcept;

  /// Traverses the graph to find nodes
  /// that have more than one parent.
  /// Common nodes are encountered breadth-first,
  /// and they are unique.
  ///
  /// @param[out] common_gates Gates with more than one parent.
  /// @param[out] common_variables Common variables.
  ///
  /// @note Constant nodes are not expected to be operated.
  ///
  /// @warning Node visit information must be clear.
  void GatherCommonNodes(
      std::vector<IGateWeakPtr>* common_gates,
      std::vector<std::weak_ptr<Variable> >* common_variables) noexcept;

  /// Tries to simplify the graph by removing redundancies
  /// generated by a common node.
  ///
  /// @param[in] common_node A node with more than one parent.
  template<class N>
  void ProcessCommonNode(const std::weak_ptr<N>& common_node) noexcept;

  /// Propagates failure of the node
  /// by setting its ancestors' optimization values to 1
  /// if they fail according to their Boolean logic.
  ///
  /// @param[in] node The node that fails.
  ///
  /// @returns Total multiplicity of the node.
  int PropagateFailure(const NodePtr& node) noexcept;

  /// Collects failure destinations
  /// and marks non-redundant nodes.
  /// The optimization value for non-redundant nodes are set to 2.
  /// The optimization value for non-removal parent nodes are set to 3.
  ///
  /// @param[in] gate The non-failed gate which sub-graph is to be traversed.
  /// @param[in] index The index of the failed node.
  /// @param[in,out] destinations Destinations of the failure.
  ///
  /// @returns The number of encounters with the destinations.
  int CollectFailureDestinations(
      const IGatePtr& gate,
      int index,
      std::map<int, IGateWeakPtr>* destinations) noexcept;

  /// Detects if parents of a node are redundant.
  /// If there are redundant parents,
  /// depending on the logic of the parent,
  /// the node is removed from the parent
  /// unless it is also in the destination set with specific logic.
  /// In the latter case, the parent is removed from the destinations.
  ///
  /// @param[in] node The common node.
  /// @param[in,out] destinations A set of destination gates.
  ///
  /// @note Constant gates are registered for removal.
  /// @note Null type gates are registered for removal.
  void ProcessRedundantParents(
      const NodePtr& node,
      std::map<int, IGateWeakPtr>* destinations) noexcept;

  /// Transforms failure destination
  /// according to the logic and the common node.
  ///
  /// @param[in] node The common node.
  /// @param[in] destinations Destination gates for failure.
  ///
  /// @warning This function will replace the root gate of the graph
  ///          if it is the failure destination.
  template<class N>
  void ProcessFailureDestinations(
      const std::shared_ptr<N>& node,
      const std::map<int, IGateWeakPtr>& destinations) noexcept;

  /// The Shannon decomposition for common nodes in the Boolean graph.
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
  /// @warning Gate optimization values are used.
  /// @warning Node visit information is used.
  /// @warning Gate marks are used.
  bool DecomposeCommonNodes() noexcept;

  /// Processes common nodes in decomposition setups.
  /// This function only works with DecomposeCommonNodes()
  /// because it requires a specific setup of
  /// optimization values and visit information of nodes.
  /// These setups are assumed
  /// to be provided by the DecomposeCommonNodes().
  ///
  /// @param[in] common_node The common node.
  ///
  /// @returns true if the decomposition setups are found and processed.
  ///
  /// @warning Gate optimization values are manipulated.
  bool ProcessDecompositionCommonNode(
      const std::weak_ptr<Node>& common_node) noexcept;

  /// Marks destinations for common node decomposition.
  /// The optimization value of the ancestors of the common node
  /// is marked with the index of the common node.
  ///
  /// @param[in] parent The parent or ancestor of the common node.
  /// @param[in] index The positive index of the common node.
  ///
  /// @warning The gate optimization value fields are changed.
  ///          It is expected that no ancestor gate has the specific opti-value
  ///          before the call of this function.
  ///          Otherwise, the logic of the algorithm is messed up and invalid.
  void MarkDecompositionDestinations(const IGatePtr& parent,
                                     int index) noexcept;

  /// Processes decomposition destinations
  /// with the decomposition setups.
  ///
  /// @param[in] node The common node under consideration.
  /// @param[in] dest The set of destination parents.
  ///
  /// @returns true if the graph is changed by processing.
  ///
  /// @warning Gate marks are used to traverse subgraphs in linear time.
  /// @warning Gate optimization values are used to detect ancestor.
  /// @warning Gate visit time information is used to detect shared nodes.
  bool ProcessDecompositionDestinations(
      const NodePtr& node,
      const std::vector<IGateWeakPtr>& dest) noexcept;

  /// Processes decomposition ancestors
  /// in the link to the decomposition destinations.
  /// Common node's parents shared outside of the subgraph
  /// may get cloned to not mess the whole graph.
  ///
  /// @param[in] ancestor The parent or ancestor of the common node.
  /// @param[in] node The common node under consideration.
  /// @param[in] state The constant state to be propagated.
  /// @param[in] visit_bounds The main graph's visit enter and exit times.
  /// @param[in,out] clones Clones of common parents in the subgraph.
  ///
  /// @returns true if the parent is reached and processed.
  ///
  /// @warning Gate marks are used to traverse subgraphs in linear time.
  ///          Gate marks must be clear for the subgraph for the first call.
  /// @warning Gate optimization values are used to detect ancestor.
  /// @warning Gate visit time information is used to detect shared nodes.
  bool ProcessDecompositionAncestors(
      const IGatePtr& ancestor,
      const NodePtr& node,
      bool state,
      const std::pair<int, int>& visit_bounds,
      std::unordered_map<int, IGatePtr>* clones) noexcept;

  /// Replaces one gate in the graph with another.
  ///
  /// @param[in,out] gate An existing gate to be replaced.
  /// @param[in,out] replacement A gate that will replace the old gate.
  ///
  /// @note The sign of the existing gate as an argument
  ///       is transfered to the replacement gate.
  ///
  /// @note If any parent becomes constant or NULL type,
  ///       the parent is registered for removal.
  void ReplaceGate(const IGatePtr& gate, const IGatePtr& replacement) noexcept;

  BooleanGraph* graph_;  ///< The Boolean graph to preprocess.
  int root_sign_;  ///< The negative or positive sign of the root node.
  /// Container for constant gates to be tracked and cleaned by algorithms.
  /// These constant gates are created
  /// because of complement or constant descendants.
  std::vector<IGateWeakPtr> const_gates_;
  /// Container for NULL type gates to be tracked and cleaned by algorithms.
  /// NULL type gates are created by coherent gates with only one argument.
  std::vector<IGateWeakPtr> null_gates_;
};

}  // namespace scram

#endif  // SCRAM_SRC_PREPROCESSOR_H_
