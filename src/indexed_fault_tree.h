/// @file indexed_fault_tree.h
/// A fault tree with event and gate indices instead of id names.
#ifndef SCRAM_SRC_INDEXED_FAULT_TREE_H_
#define SCRAM_SRC_INDEXED_FAULT_TREE_H_

#include <map>
#include <set>
#include <string>

#include <boost/unordered_map.hpp>

#include "event.h"
#include "indexed_gate.h"

typedef boost::shared_ptr<scram::Gate> GatePtr;

namespace scram {

/// @class IndexedFaultTree
/// This class should provide simpler representation of a fault tree
/// that takes into account the indices of events instead of ids and pointers.
class IndexedFaultTree {
  // This class should:
  // 1. Use indices of events.
  // 2. Propagate fault tree constants (House events).
  // 3. Unroll complex gates like XOR and VOTE.
  // 4. Pre-process the tree to simplify the structure.
  // 5. Take into account non-coherent logic for the fault tree.
  // Ideally, the resulting tree should have only OR and AND gates with
  // basic event and gate indices only.
  // The indices of gates may change, but the indices of basic events must
  // not change.
 public:
  /// Constructs a simplified fault tree.
  /// @param[in] top_event_id The index of the top event of this tree.
  IndexedFaultTree(int top_event_id, int limit_order);

  /// Removes all newly allocated gates to describe the simplified tree.
  ~IndexedFaultTree();

  /// Creates indexed gates with basic and house event indices as children.
  /// This function also simplifies the tree to simple gates.
  /// @param[in] int_to_inter Container of gates and thier indices including
  ///                         the top gate.
  /// @param[in] all_to_int Container of all events in this tree to index
  ///                       children of the gates.
  void InitiateIndexedFaultTree(
      const boost::unordered_map<int, GatePtr>& int_to_inter,
      const boost::unordered_map<std::string, int>& all_to_int);

  /// Remove all house events by propagating them as constants in Boolean
  /// equation.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events);

  /// Performs processing of a fault tree.
  void ProcessIndexedFaultTree();

  /// Finds minimal cut sets.
  /// @warning This is experimental for coherent trees only.
  void FindMcs();

  inline const std::set<int>& GateChildren(int index) {
    return indexed_gates_.find(index)->second->children();
  }

  /// @returns Numbered type of the gate: 1 is OR, 2 is AND.
  inline int GateType(int index) {
    return indexed_gates_.find(index)->second->type();
  }

  /// @returns Generated minimal cut sets with basic event indices.
  inline const std::vector< std::set<int> >& GetGeneratedMcs() {
    return imcs_;
  }

 private:
  /// Start unrolling gates to simplify gates to OR and AND gates.
  void StartUnrollingGates();

  /// Unrolls the top gate.
  /// @param[out] top_gate The top gate to start unrolling with.
  void UnrollTopGate(IndexedGate* top_gate);

  /// Unrolls a gate.
  /// @param[out] parent_gate The current parent of the gates to process.
  /// @param[out] unrolled_gate To keep track of already unrolled gates.
  void UnrollGates(IndexedGate* parent_gate, std::set<int>* unrolled_gates);

  /// Unrolls a gate with XOR logic.
  /// @param[out] gate The gate to unroll.
  void UnrollXorGate(IndexedGate* gate);

  /// Unrolls a gate with "atleast" gate and vote number.
  /// @param[out] gate The atleast gate to unroll.
  void UnrollAtleastGate(IndexedGate* gate);

  /// Remove all house events from a given gate.
  /// After this method, there should not be any unity or null gates.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  /// @param[out] gate The final resultant processed gate.
  /// @param[out] processed_gates The gates that has already been processed.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events,
                          IndexedGate* gate,
                          std::set<int>* processed_gates);

  /// Propagates complements of child gates down to basic events
  /// in order to remove any NOR or NAND logic from the tree.
  /// @param[out] gate The starting gate to traverse the tree. This is for
  ///                 recursive purposes. The sign of this passed gate
  ///                 is unknown for the function, so it must be sanitized
  ///                 for a top event to function correctly.
  /// @param[out] gate_complements The complements of gates already processed.
  /// @param[out] processed_gates The gates that has already been processed.
  void PropagateComplements(IndexedGate* gate,
                            std::map<int, int>* gate_complements,
                            std::set<int>* processed_gates);

  /// This method pre-processes the tree by doing Boolean algebra.
  /// At this point all gates are expected to be either OR type or AND type.
  /// This function merges similar gates and may produce null gates.
  /// @param[out] gate The starting gate to traverse the tree. This is for
  ///                 recursive purposes.
  /// @param[out] processed_gates The gates that has already been processed.
  void PreprocessTree(IndexedGate* gate, std::set<int>* processed_gates);

  /// Process null gates.
  /// After this function, there should not be null gates resulting
  /// from previous processing steps.
  /// @param[out] gate The starting gate to traverse the tree. This is for
  ///                 recursive purposes.
  /// @param[out] processed_gates The gates that has already been processed.
  void ProcessNullGates(IndexedGate* gate, std::set<int>* processed_gates);

  /// Expands And layer in preprocessed fault tree.
  /// @param[in] gate The AND gate to be processed.
  /// @param[out] next_layer Container for processing next layer AND gates.
  /// @returns the index of the new expanded gate that should go in place of
  ///          the provided gate.
  /// @returns 0 if the provided gate contains only basic events.
  /// @returns -1 if the provided gate contains no children.
  int ExpandAndLayer(const IndexedGate* gate, std::vector<int>* next_layer);

  /// Finds minimal cut sets from cut sets.
  /// Applys rule 4 to reduce unique cut sets to minimal cut sets.
  /// @param[in] cut_sets Cut sets with primary events.
  /// @param[in] mcs_lower_order Reference minimal cut sets of some order.
  /// @param[in] min_order The order of sets to become minimal.
  /// @param[out] imcs Min cut sets with indices of events.
  /// @note T_avg(N^3 + N^2*logN + N*logN) = O_avg(N^3)
  void FindMcs(const std::vector< const std::set<int>* >& cut_sets,
               const std::vector< std::set<int> >& mcs_lower_order,
               int min_order,
               std::vector< std::set<int> >* imcs);

  int top_event_index_;  ///< The index of the top gate of this tree.
  /// All gates of this tree including newly created ones.
  boost::unordered_map<int, IndexedGate*> indexed_gates_;
  int top_event_sign_;  ///< The negative or positive sign of the top event.
  int new_gate_index_;  ///< Index for a new gate.
  std::vector< std::set<int> > imcs_;  // Min cut sets with indexed events.
  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_FAULT_TREE_H_
