/// @file indexed_fault_tree.h
/// A fault tree analysis facility with event and gate indices instead
/// of id names. This facility is designed to work with FaultTreeAnalysis class.
#ifndef SCRAM_SRC_INDEXED_FAULT_TREE_H_
#define SCRAM_SRC_INDEXED_FAULT_TREE_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "indexed_gate.h"

class IndexedFaultTreeTest;

namespace scram {

class Gate;
class Formula;
class Mocus;

/// @class IndexedFaultTree
/// This class provides simpler representation of a fault tree
/// that takes into account the indices of events instead of ids and pointers.
/// The class provides main preprocessing operations over a fault tree
/// to generate minimal cut sets.
class IndexedFaultTree {
  friend class ::IndexedFaultTreeTest;
  friend class Mocus;

 public:
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs a simplified fault tree with indices of nodes.
  /// @param[in] top_event_id The index of the top event of this tree.
  explicit IndexedFaultTree(int top_event_id);

  /// Creates indexed gates with basic and house event indices as children.
  /// Nested gates are flattened and given new indices.
  /// It is assumed that indices are sequential starting from 1.
  /// @param[in] int_to_inter Container of gates and their indices including
  ///                         the top gate.
  /// @param[in] ccf_basic_to_gates CCF basic events that are converted to
  ///                               gates.
  /// @param[in] all_to_int Container of all events in this tree to index
  ///                       children of the gates.
  void InitiateIndexedFaultTree(
      const boost::unordered_map<int, GatePtr>& int_to_inter,
      const std::map<std::string, int>& ccf_basic_to_gates,
      const boost::unordered_map<std::string, int>& all_to_int);

  /// Remove all house events by propagating them as constants in Boolean
  /// equation.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events);

  /// Performs processing of a fault tree to simplify the structure to
  /// normalized (OR/AND gates only), modular, positive-gate-only indexed fault
  /// tree.
  /// @param[in] num_basic_events The number of basic events. This information
  ///                             is needed to optimize the tree traversal
  ///                             with certain expectation.
  void ProcessIndexedFaultTree(int num_basic_events);

 private:
  typedef boost::shared_ptr<IndexedGate> IndexedGatePtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Mapping to string gate types to enum gate types.
  static const std::map<std::string, GateType> kStringToType_;

  /// @returns true If the given index belongs to an indexed gate.
  /// @param[in] index Positive index.
  /// @warning The actual existance of the indexed gate is not guaranteed.
  inline bool IsGateIndex(int index) const {
    assert(index > 0);
    return index >= kGateIndex_;
  }

  /// Adds a new indexed gate into the indexed fault tree's gate container.
  /// @param[in] gate A new indexed gate.
  inline void AddGate(const IndexedGatePtr& gate) {
    assert(!indexed_gates_.count(gate->index()));
    indexed_gates_.insert(std::make_pair(gate->index(), gate));
  }

  /// Commonly used function to get indexed gates from indices.
  /// @param[in] index Positive index of a gate.
  /// @returns The pointer to the requested indexed gate.
  inline const IndexedGatePtr& GetGate(int index) const {
    assert(index > 0);
    assert(index >= kGateIndex_);
    assert(indexed_gates_.count(index));
    return indexed_gates_.find(index)->second;
  }

  /// Processes a formula into a new indexed gates.
  /// @param[in] index The index to be assigned to the new indexed gate.
  /// @param[in] formula The formula to be converted into a gate.
  /// @param[in] ccf_basic_to_gates CCF basic events that are converted to
  ///                               gates.
  /// @param[in] all_to_int Container of all events in this tree to index
  ///                       children of the gates.
  void ProcessFormula(int index,
                      const FormulaPtr& formula,
                      const std::map<std::string, int>& ccf_basic_to_gates,
                      const boost::unordered_map<std::string, int>& all_to_int);

  /// Starts normalizing gates to simplify gates to OR, AND gates.
  /// This function uses parent information of each gate, so the tree must
  /// be initialized before a call of this function.
  /// New gates are created upon normalizing complex gates, such as XOR.
  /// @warning NUll gates are not handled except for the top gate.
  void NormalizeGates();

  /// Traverses the tree to gather information about parents of indexed gates.
  /// This information might be needed for other algorithms because
  /// due to processing of the tree, the shape and nodes may change.
  /// @param[in] parent_gate The parent to start information gathering.
  void GatherParentInformation(const IndexedGatePtr& parent_gate);

  /// Notifies all parents of negative gates, such as NOT, NOR, and NAND before
  /// transforming these gates into basic gates of OR and AND. The child gates
  /// are swaped with a negative sign.
  /// @param[in] gate The gate to start processing.
  /// @warning This function does not change the types of gates.
  /// @warning The top gate does not have parents, so it is not handled here.
  void NotifyParentsOfNegativeGates(const IndexedGatePtr& gate);

  /// Normalizes complex gates into OR, AND gates.
  /// @param[in,out] gate The gate to be processed.
  /// @warning The parents of negative gates are assumed to be notified about
  ///          the change of their children types.
  /// @warning NULL gates are not handled.
  void NormalizeGate(const IndexedGatePtr& gate);

  /// Normalizes a gate with XOR logic. This is a helper function for the main
  /// gate normalization function.
  /// @param[in,out] gate The gate to normalize.
  void NormalizeXorGate(const IndexedGatePtr& gate);

  /// Normalizes an ATLEAST gate with a vote number. The gate is turned into
  /// an OR gate of recursively normalized ATLEAST and AND child gates according
  /// to the formula K/N(x, y_i) = OR(AND(x, K-1/N-1(y_i)), K/N-1(y_i))) with
  /// y_i being the rest of formula variables, which exclude x.
  /// This representation is more friendly to other preprocessing and analysis
  /// techniques than the alternative, which is OR of AND gates of combinations.
  /// @param[in,out] gate The atleast gate to normalize.
  void NormalizeAtleastGate(const IndexedGatePtr& gate);

  /// Remove all house events from a given gate according to the Boolean logic.
  /// The structure of the tree should not be pre-processed before this
  /// operation; that is, this is the first operation that is done after
  /// creation of an indexed fault tree.
  /// After this function, there should not be any unity or null gates because
  /// of house events.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  /// @param[in,out] gate The final resultant processed gate.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events,
                          const IndexedGatePtr& gate);

  /// Changes the state of a gate or passes a constant child to be removed
  /// later. The function determines its actions depending on the type of
  /// a gate and state of a child; however, the sign of the index is ignored.
  /// The caller of this function must ensure that the state corresponds to the
  /// sign of the child index.
  /// The type of the gate may change, but it will only be valid after the
  /// to-be-erased children are handled properly.
  /// @param[in,out] gate The parent gate that contains the children.
  /// @param[in] child The constant child under consideration.
  /// @param[in] state False or True constant state of the child.
  /// @param[in,out] to_erase The set of children to erase from the parent gate.
  /// @returns true if the passed gate has become constant due to its child.
  /// @returns false if the parent still valid for further operations.
  bool ProcessConstantChild(const IndexedGatePtr& gate, int child,
                            bool state, std::vector<int>* to_erase);

  /// Removes a set of children from a gate taking into account the logic.
  /// This is a helper function for NULL and UNITY propagation on the tree.
  /// If the final gate is empty, its state is turned into NULL or UNITY
  /// depending on the logic of the gate and the logic of the constant
  /// propagation.
  /// The parent information is not updated for the child.
  /// @param[in,out] gate The gate that contains the children to be removed.
  /// @param[in] to_erase The set of children to erase from the parent gate.
  void RemoveChildren(const IndexedGatePtr& gate,
                      const std::vector<int>& to_erase);

  /// Removes NULL and UNITY state child gates. The parent gate can be of any
  /// type. Because of the constant children, the parent gate itself may turn
  /// constant in some cases.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  /// @warning There should not be negative gate children.
  /// @warning There still may be only one constant state gate which is the root
  ///          of the tree. This must be handled separately.
  bool RemoveConstGates(const IndexedGatePtr& gate);

  /// Propagates complements of child gates down to basic events
  /// in order to remove any negative logic from the fault tree's gates.
  /// The tree must contain only OR and AND type gates. All NULL type gates must
  /// be removed from the tree. Other complex gates must be preprocessed.
  /// The resulting tree will contain only positive gates, OR and AND.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes. The sign of this passed gate
  ///                     is unknown for the function, so it must be sanitized
  ///                     for a top event to function correctly.
  /// @param[in,out] gate_complements The processed complements of gates.
  void PropagateComplements(const IndexedGatePtr& gate,
                            std::map<int, int>* gate_complements);

  /// Removes child gates of NULL type, which means these child gates have
  /// only one child. That one grandchild is transfered to the parent gate,
  /// and the original child gate is removed from the parent gate.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  /// @warning There still may be only one NULL type gate which is the root
  ///          of the tree. This must be handled separately.
  bool RemoveNullGates(const IndexedGatePtr& gate);

  /// Pre-processes the fault tree by doing the simplest Boolean algebra.
  /// Positive children with the same OR or AND gates as parents are coalesced.
  /// This function merges similar logic gates of NAND and NOR as well.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  /// @warning NULL or UNITY state gates may emerge because of this processing.
  /// @warning NULL type gates are not handled by this function.
  /// @warning Module child gates are omitted from coalescing.
  bool JoinGates(const IndexedGatePtr& gate);

  /// Traverses the indexed fault tree to detect modules.
  /// @param[in] num_basic_events The number of basic events in the tree.
  void DetectModules(int num_basic_events);

  /// Traverses the given gate and assigns time of visit to nodes.
  /// @param[in] time The current time.
  /// @param[in,out] gate The gate to traverse and assign time to.
  /// @param[in,out] visit_basics The recordings for basic events.
  /// @returns The final time of traversing.
  int AssignTiming(int time, const IndexedGatePtr& gate, int visit_basics[][2]);

  /// Determines modules from original gates that have been already timed.
  /// This function can also create new modules from the existing tree.
  /// @param[in,out] gate The gate to test for modularity.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in,out] visited_gates Container of visited gates with
  ///                              min and max time of visits of the subtree.
  void FindOriginalModules(const IndexedGatePtr& gate,
                           const int visit_basics[][2],
                           std::map<int, std::pair<int, int> >* visited_gates);

  /// Creates a new module as a child of an existing gate. The existing
  /// children of the original gate are used to create the new module.
  /// The module is added in the module and gate databases.
  /// If the new module must contain all the children, the original gate is
  /// turned into a module.
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] children Modular children to be added into the new module.
  void CreateNewModule(const IndexedGatePtr& gate,
                       const std::vector<int>& children);

  /// Checks if a group of modular children share anything with non-modular
  /// children. If so, then the modular children are not actually modular, and
  /// that children are removed from modular containers.
  /// This is due to chain of events that are shared between modular and
  /// non-modular children.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in] visited_gates Visit max and min time recordings for gates.
  /// @param[in,out] modular_children Candidates for modular grouping.
  /// @param[in,out] non_modular_children Non modular children.
  void FilterModularChildren(
      const int visit_basics[][2],
      const std::map<int, std::pair<int, int> >& visited_gates,
      std::vector<int>* modular_children,
      std::vector<int>* non_modular_children);

  /// Clears visit time information from all indexed gates that are presently
  /// in this fault tree's container. Any member function updating and using the
  /// visit information of gates must ensure to clean visit times before running
  /// algorithms. However, cleaning after finishing algorithms is not mandatory.
  void ClearGateVisits();

  int top_event_index_;  ///< The index of the top gate of this tree.
  const int kGateIndex_;  ///< The starting gate index for gate identification.
  /// All gates of this tree including newly created ones.
  boost::unordered_map<int, IndexedGatePtr> indexed_gates_;
  int top_event_sign_;  ///< The negative or positive sign of the top event.
  int new_gate_index_;  ///< Index for a new gate.
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_FAULT_TREE_H_
