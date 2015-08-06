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
/// A collection of fault tree preprocessing algorithms that simplify
/// fault trees for analysis.
#ifndef SCRAM_SRC_PREPROCESSOR_H_
#define SCRAM_SRC_PREPROCESSOR_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>

#include "boolean_graph.h"

class PreprocessorTest;

namespace scram {

/// @class Preprocessor
/// The class provides main preprocessing operations over a Boolean graph
/// to simplify the fault tree and to generate minimal cut sets more
/// efficiently.
class Preprocessor {
  friend class ::PreprocessorTest;

 public:
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs a preprocessor of an indexed fault tree.
  ///
  /// @param[in] fault_tree The fault tree to be preprocessed.
  ///
  /// @warning There should not be another shared pointer to the top gate
  ///          outside of the passed indexed fault tree. Upon preprocessing a
  ///          new top gate may be assigned to the fault tree, and if there is
  ///          an extra pointer to the previous top gate outside of the fault
  ///          tree, the destructor will not be called as expected by the
  ///          preprocessing algorithms, which will mess the new structure of
  ///          the indexed fault tree.
  explicit Preprocessor(BooleanGraph* fault_tree);

  /// Performs processing of a fault tree to simplify the structure to
  /// normalized (OR/AND gates only), modular, positive-gate-only indexed fault
  /// tree.
  ///
  /// @warning There should not be another smart pointer to the indexed top
  ///          gate of the indexed fault tree outside of the tree.
  void ProcessFaultTree();

 private:
  typedef boost::shared_ptr<Node> NodePtr;
  typedef boost::shared_ptr<IGate> IGatePtr;
  typedef boost::shared_ptr<Variable> VariablePtr;
  typedef boost::shared_ptr<Constant> ConstantPtr;

  /// Normalizes the gates of the whole indexed fault tree into OR, AND gates.
  ///
  /// @note The negation of the top gate is saved and handled in a special
  ///       because it does not have a parent.
  /// @note New gates are created upon normalization of complex gates like XOR.
  /// @note This function is meant to be called only once.
  ///
  /// @warning NULL type gates are left untouched for coalescing functions.
  void NormalizeGates();

  /// Notifies all parents of negative gates, such as NOT, NOR, and NAND, before
  /// transforming these gates into basic gates of OR and AND. The argument gates
  /// are swaped with a negative sign.
  ///
  /// @param[in] gate The gate to start processing.
  ///
  /// @note This function is a helper function for NormalizeGates().
  ///
  /// @warning Gate marks must be clear.
  /// @warning This function does not change the types of gates.
  /// @warning The top gate does not have parents, so it is not handled here.
  void NotifyParentsOfNegativeGates(const IGatePtr& gate);

  /// Normalizes complex gates into OR, AND gates.
  ///
  /// @param[in,out] gate The gate to be processed.
  ///
  /// @note This is a helper function for NormalizeGates().
  ///
  /// @warning Gate marks must be clear.
  /// @warning The parents of negative gates are assumed to be notified about
  ///          the change of their argument types.
  /// @warning NULL gates are not handled.
  void NormalizeGate(const IGatePtr& gate);

  /// Normalizes a gate with XOR logic. This is a helper function for the main
  /// gate normalization function.
  ///
  /// @param[in,out] gate The gate to normalize.
  ///
  /// @note This is a helper function for NormalizeGate.
  void NormalizeXorGate(const IGatePtr& gate);

  /// Normalizes an ATLEAST gate with a vote number. The gate is turned into
  /// an OR gate of recursively normalized ATLEAST and AND arg gates according
  /// to the formula K/N(x, y_i) = OR(AND(x, K-1/N-1(y_i)), K/N-1(y_i))) with
  /// y_i being the rest of formula variables, which exclude x.
  /// This representation is more friendly to other preprocessing and analysis
  /// techniques than the alternative, which is OR of AND gates of combinations.
  ///
  /// @param[in,out] gate The atleast gate to normalize.
  ///
  /// @note This is a helper function for NormalizeGate.
  void NormalizeAtleastGate(const IGatePtr& gate);

  /// Propagates constant gates buttom-up. This is a helper function for
  /// algorithms that may produce and need to remove constant gates.
  ///
  /// @param[in,out] gate The gate that has become constant.
  ///
  /// @note This function works with NULL type gate propagation function to
  ///       cleanup the structure of the tree.
  ///
  /// @warning All parents of the gate will be deleted, so the gate itself may
  ///          get deleted unless it is the top gate.
  void PropagateConstGate(const IGatePtr& gate);

  /// Propagate NULL type gates buttom-up. This is a helper function for
  /// algorithms that may produce and need to remove NULL type gates as soon
  /// as it is detected.
  ///
  /// @param[in,out] gate The gate that is NULL type.
  ///
  /// @note This function works with constant state gate propagation function to
  ///       cleanup the structure of the tree.
  ///
  /// @warning All parents of the gate will be deleted, so the gate itself may
  ///          get deleted unless it is the top gate.
  void PropagateNullGate(const IGatePtr& gate);

  /// Removes all constants and constant gates from a given sub-tree according
  /// to the Boolean logic of the gates. This algorithm is top-down search for
  /// all constants. It is less efficient than a targeted bottom-up propagation
  /// for a specific constant. Therefore, this function is used to get rid of
  /// all constants without knowing where they are or what they are.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  ///
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @note This is one of the first preprocessing steps. Other algorithms are
  ///       safe to assume that there are no house events in the faul tree.
  ///       Only possible constant nodes are gates that turn NULL or UNITY sets.
  ///
  /// @warning Gate marks must be clear.
  /// @warning There still may be only one constant state gate which is the root
  ///          of the tree. This must be handled separately.
  bool PropagateConstants(const IGatePtr& gate);

  /// Changes the state of a gate or passes a constant argument to be removed
  /// later. The function determines its actions depending on the type of
  /// a gate and state of an argument; however, the index sign is ignored.
  /// The caller of this function must ensure that the state corresponds to the
  /// sign of the argument index.
  /// The type of the gate may change, but it will only be valid after the
  /// to-be-erased arguments are handled properly.
  ///
  /// @param[in,out] gate The parent gate that contains the arguments.
  /// @param[in] arg The constant argument under consideration.
  /// @param[in] state False or True constant state of the argument.
  /// @param[in,out] to_erase The set of arguments to erase from the parent gate.
  ///
  /// @returns true if the passed gate has become constant due to its argument.
  /// @returns false if the parent still valid for further operations.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  bool ProcessConstantArg(const IGatePtr& gate, int arg,
                          bool state, std::vector<int>* to_erase);

  /// Removes a set of arguments from a gate taking into account the logic.
  /// This is a helper function for NULL and UNITY propagation on the tree.
  /// If the final gate is empty, its state is turned into NULL or UNITY
  /// depending on the logic of the gate and the logic of the constant
  /// propagation.
  ///
  /// @param[in,out] gate The gate that contains the arguments to be removed.
  /// @param[in] to_erase The set of arguments to erase from the parent gate.
  ///
  /// @note This is a helper function that propagates constants, so it is
  ///       coupled with the logic of the constant propagation algorithms.
  void RemoveArgs(const IGatePtr& gate, const std::vector<int>& to_erase);

  /// Propagates complements of argument gates down to leafs according to
  /// the De Morgan's law in order to remove any negative logic from the fault
  /// tree's gates. The resulting tree will contain only positive gates, OR
  /// and AND. After this function, the Boolean graph is in negation normal
  /// form.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes. The sign of this passed gate
  ///                     is unknown for the function, so it must be sanitized
  ///                     for a top event to function correctly.
  /// @param[in,out] gate_complements The processed complements of gates.
  ///
  /// @note The tree must be normalized. It must contain only OR and AND gates.
  ///
  /// @warning Gate marks must be clear.
  /// @warning If the root gate has a negative sign, it must be handled before
  ///          calling this function. The arguments and type of the gate must
  ///          be inverted according to the logic of the root gate.
  ///
  /// @todo Module-aware complement propagation.
  void PropagateComplements(const IGatePtr& gate,
                            std::map<int, IGatePtr>* gate_complements);

  /// Removes argument gates of NULL type, which means these arg gates have
  /// only one argument. That one grand arg is transfered to the parent gate,
  /// and the original argument gate is removed from the parent gate.
  ///
  /// This is a top-down algorithm that searches for all NULL type gates, which
  /// means it is less efficient than having a specific NULL type gate
  /// propagate its argument bottom-up.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  ///
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @warning Gate marks must be clear.
  /// @warning There still may be only one NULL type gate which is the root
  ///          of the tree. This must be handled separately.
  /// @warning NULL gates that are constant are not handled and left for
  ///          constant propagation functions.
  bool RemoveNullGates(const IGatePtr& gate);

  /// Pre-processes the fault tree by doing the simplest Boolean algebra.
  /// Positive arguments with the same OR or AND gates as parents are coalesced.
  /// This function merges similar logic gates of NAND and NOR as well.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  ///
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @note Constant state gates may be generated upon joining. These gates
  ///       are registered for future processing.
  /// @note It is impossible that this function generates NULL type gates.
  /// @note Module gates are omitted from coalescing to preserve them.
  bool JoinGates(const IGatePtr& gate);

  /// Traverses the indexed fault tree to detect modules. Modules are
  /// independent sub-trees without common nodes with the rest of the tree.
  void DetectModules();

  /// Traverses the given gate and assigns time of visit to nodes.
  ///
  /// @param[in] time The current time.
  /// @param[in,out] gate The gate to traverse and assign time to.
  ///
  /// @returns The final time of traversing.
  int AssignTiming(int time, const IGatePtr& gate);

  /// Determines modules from original gates that have been already timed.
  /// This function can also create new modules from the existing tree.
  ///
  /// @param[in,out] gate The gate to test for modularity.
  void FindModules(const IGatePtr& gate);

  /// Creates a new module as an argument of an existing gate if the logic of
  /// the existing parent gate allows a sub-module. The existing
  /// arguments of the original gate are used to create the new module.
  /// If the new module must contain all the arguments, the original gate is
  /// asserted to be a module, and no operation is performed.
  ///
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] args Modular arguments to be added into the new module.
  ///
  /// @returns Pointer to the new module if it is created.
  IGatePtr CreateNewModule(const IGatePtr& gate,
                           const std::vector<std::pair<int, NodePtr> >& args);

  /// Checks if a group of modular arguments share anything with non-modular
  /// arguments. If so, then the modular arguments are not actually modular, and
  /// that arguments are removed from modular containers.
  /// This is due to chain of events that are shared between modular and
  /// non-modular arguments.
  ///
  /// @param[in,out] modular_args Candidates for modular grouping.
  /// @param[in,out] non_modular_args Non modular arguments.
  void FilterModularArgs(
      std::vector<std::pair<int, NodePtr> >* modular_args,
      std::vector<std::pair<int, NodePtr> >* non_modular_args);

  /// Groups modular arguments by their common elements. The gates created with
  /// these modular arguments are guaranteed to be independent modules.
  ///
  /// @param[in] modular_args Candidates for modular grouping.
  /// @param[out] groups Grouped modular arguments.
  void GroupModularArgs(
      const std::vector<std::pair<int, NodePtr> >& modular_args,
      std::vector<std::vector<std::pair<int, NodePtr> > >* groups);

  /// Creates new module gates from groups of modular arguments if the logic of
  /// the parent gate allows sub-modules. The existing
  /// arguments of the original gate are used to create the new modules.
  /// If all the parent gate arguments are modular and within one group,
  /// the parent gate is asserted to be a module gate, and no operation is
  /// performed.
  ///
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] modular_args All the modular arguments.
  /// @param[in] groups Grouped modular arguments.
  void CreateNewModules(
      const IGatePtr& gate,
      const std::vector<std::pair<int, NodePtr> >& modular_args,
      const std::vector<std::vector<std::pair<int, NodePtr> > >& groups);

  /// Propagates failures of common nodes to detect redundancy. The fault tree
  /// structure is optimized by removing the reduncies if possible. This
  /// optimization helps reduce the number of common nodes.
  void BooleanOptimization();

  /// Traversers the fault tree to find nodes that have more than one parent.
  /// Common nodes are encountered breadth-first, and they are unique.
  ///
  /// @param[out] common_gates Gates with more than one parent.
  /// @param[out] common_variables Common variables.
  ///
  /// @note Constant nodes are not expected to be operated.
  void GatherCommonNodes(
      std::vector<boost::weak_ptr<IGate> >* common_gates,
      std::vector<boost::weak_ptr<Variable> >* common_variables);

  /// Tries to simplify the fault tree by removing redundancies generated by
  /// a common node.
  ///
  /// @param[in] common_node A node with more than one parent.
  template<class N>
  void ProcessCommonNode(const boost::weak_ptr<N>& common_node);

  /// Propagates failure of the node by setting its ancestors' optimization
  /// values to 1 if they fail according to their Boolean logic.
  ///
  /// @param[in] node The node that fails.
  ///
  /// @returns Total multiplicity of the node.
  int PropagateFailure(const NodePtr& node);

  /// Collects failure destinations and marks non-redundant nodes.
  /// The optimization value for non-redundant nodes are set to 2.
  /// The optimization value for non-removal parent nodes are set to 3.
  ///
  /// @param[in] gate The non-failed gate which sub-tree is to be traversed.
  /// @param[in] index The index of the failed node.
  /// @param[in,out] destinations Destinations of the failure.
  ///
  /// @returns The number of encounters with the destinations.
  int CollectFailureDestinations(
      const IGatePtr& gate,
      int index,
      std::map<int, boost::weak_ptr<IGate> >* destinations);

  /// Detects if parents of a node are redundant. If there are redundant
  /// parents, depending on the logic of the parent, the node is removed from
  /// the parent unless it is also in the destination set. In the latter case,
  /// the parent is removed from the destinations.
  ///
  /// @param[in] node The common node.
  /// @param[in,out] destinations A set of destination gates.
  ///
  /// @returns true if some of the redundant parents turned into a constant.
  ///
  /// @warning This cleanup function may generate NULL type gates.
  bool ProcessRedundantParents(
      const NodePtr& node,
      std::map<int, boost::weak_ptr<IGate> >* destinations);

  /// Transforms failure destination according to the logic and the common node.
  ///
  /// @param[in] node The common node.
  /// @param[in] destinations Destination gates for failure.
  template<class N>
  void ProcessFailureDestinations(
      const boost::shared_ptr<N>& node,
      const std::map<int, boost::weak_ptr<IGate> >& destinations);

  /// Detects and replaces multiple definitions of gates. Gates with the same
  /// logic and inputs but different indices are considered redundant.
  ///
  /// @param[in] gate The gate to traverse the sub-tree.
  /// @param[in,out] gates Ordered gates by their type.
  ///
  /// @returns true if multiple definitions are found and replaced.
  bool DetectMultipleDefinitions(const IGatePtr& gate,
                                 std::vector<std::vector<IGatePtr> >* gates);

  /// Sets the visit marks to False for all indexed gates, starting from the top
  /// gate, that have been visited top-down. Any member function updating and
  /// using the visit marks of gates must ensure to clean visit marks before
  /// running algorithms. However, cleaning after finishing algorithms is not
  /// mandatory.
  ///
  /// @warning If the marks have not been assigned in a top-down traversal, this
  ///          function will fail silently.
  void ClearGateMarks();

  /// Sets the visit marks of descendant gates to False starting from the given
  /// gate as a root. The top-down traversal marking is assumed.
  ///
  /// @param[in,out] gate The root gate to be traversed and marks.
  ///
  /// @warning If the marks have not been assigned in a top-down traversal
  ///          starting from the given gate, this function will fail silently.
  void ClearGateMarks(const IGatePtr& gate);

  /// Clears visit time information from all indexed nodes that have been
  /// visited or not. Any member function updating and using the visit
  /// information of gates must ensure to clean visit times before running
  /// algorithms. However, cleaning after finishing algorithms is not mandatory.
  void ClearNodeVisits();

  /// Clears visit information from descendant nodes starting from the given
  /// gate as a root.
  ///
  /// @param[in,out] gate The root gate to be traversed and cleaned.
  void ClearNodeVisits(const IGatePtr& gate);

  /// Clears optimization values of nodes. The optimization values are set to 0.
  /// Resets the number of failed arguments of gates.
  ///
  /// @param[in,out] gate The root gate to be traversed and cleaned.
  void ClearOptiValues(const IGatePtr& gate);

  BooleanGraph* fault_tree_;  ///< The fault tree to preprocess.
  int top_event_sign_;  ///< The negative or positive sign of the top event.
  bool constants_;  ///< Indication if there are constants in the tree.
  /// Container for constant gates to be tracked and cleaned by algorithms.
  /// These constant gates are created because of complement or constant
  /// descendants.
  std::vector<boost::weak_ptr<IGate> > const_gates_;
  /// Container for NULL type gates to be tracked and cleaned by algorithms.
  /// NULL type gates are created by coherent gates with only one argument.
  std::vector<boost::weak_ptr<IGate> > null_gates_;
};

}  // namespace scram

#endif  // SCRAM_SRC_PREPROCESSOR_H_
