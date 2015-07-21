/// @file indexed_fault_tree.h
/// A fault tree analysis facility with event and gate indices instead
/// of id names. This facility is designed to work with FaultTreeAnalysis class.
#ifndef SCRAM_SRC_INDEXED_FAULT_TREE_H_
#define SCRAM_SRC_INDEXED_FAULT_TREE_H_

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "indexed_gate.h"

namespace scram {

class Gate;
class Formula;

/// @class IndexedFaultTree
/// This class provides simpler representation of a fault tree
/// that takes into account the indices of events instead of ids and pointers.
class IndexedFaultTree {
 public:
  typedef boost::shared_ptr<IndexedGate> IndexedGatePtr;
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs a simplified fault tree with indices of nodes.
  ///
  /// @param[in] top_event_id The index of the top event of this tree.
  explicit IndexedFaultTree(int top_event_id);

  /// @returns The index of the top gate of this fault tree.
  inline int top_event_index() const { return top_event_index_; }

  /// Sets the index for the top gate.
  ///
  /// @param[in] index Positive index of the top gate.
  inline void top_event_index(int index) { top_event_index_ = index; }

  /// @returns The current top gate of the fault tree.
  inline const IndexedGatePtr& top_event() const {
    assert(indexed_gates_.count(top_event_index_));
    return indexed_gates_.find(top_event_index_)->second;
  }

  /// Creates indexed gates with basic and house event indices as children.
  /// Nested gates are flattened and given new indices.
  /// It is assumed that indices are sequential starting from 1.
  ///
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

  /// Determines the type of the index.
  ///
  /// @param[in] index Positive index.
  ///
  /// @returns true if the given index belongs to an indexed gate.
  ///
  /// @warning The actual existance of the indexed gate is not guaranteed.
  inline bool IsGateIndex(int index) const {
    assert(index > 0);
    return index >= kGateIndex_;
  }

  /// Adds a new indexed gate into the indexed fault tree's gate container.
  ///
  /// @param[in] gate A new indexed gate.
  inline void AddGate(const IndexedGatePtr& gate) {
    assert(!indexed_gates_.count(gate->index()));
    indexed_gates_.insert(std::make_pair(gate->index(), gate));
  }

  /// Commonly used function to get indexed gates from indices.
  ///
  /// @param[in] index Positive index of a gate.
  ///
  /// @returns The pointer to the requested indexed gate.
  inline const IndexedGatePtr& GetGate(int index) const {
    assert(index > 0);
    assert(index >= kGateIndex_);
    assert(indexed_gates_.count(index));
    return indexed_gates_.find(index)->second;
  }

  /// Creates a new indexed gate. The gate is added to the fault tree container.
  /// The index for the new gates are assigned sequentially and guaranteed to
  /// be unique.
  ///
  /// @param[in] type The type of the new gate.
  ///
  /// @returns Pointer to the newly created gate.
  inline IndexedGatePtr CreateGate(const GateType& type) {
    IndexedGatePtr gate(new IndexedGate(++new_gate_index_, type));
    indexed_gates_.insert(std::make_pair(gate->index(), gate));
    return gate;
  }

 private:
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Mapping to string gate types to enum gate types.
  static const std::map<std::string, GateType> kStringToType_;

  /// Processes a formula into a new indexed gates.
  ///
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

  int top_event_index_;  ///< The index of the top gate of this tree.
  const int kGateIndex_;  ///< The starting gate index for gate identification.
  /// All gates of this tree including newly created ones.
  boost::unordered_map<int, IndexedGatePtr> indexed_gates_;
  int new_gate_index_;  ///< Index for a new gate.
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_FAULT_TREE_H_
