#ifndef SCRAM_SRC_INDEXED_GATE_H_
#define SCRAM_SRC_INDEXED_GATE_H_

#include <set>
#include <string>

#include "event.h"

namespace scram {

/// @class IndexedGate
/// This gate is for use in IndexedFaultTree.
/// This gate can be only of OR and AND type.
class IndexedGate {
 public:
  /// Creates a gate with its index.
  /// @param[in] index An unique positive index of this gate.
  /// @warning The index is not validated upon instantiation.
  IndexedGate(int index);

  /// Sets the gate type information.
  /// 1 is for OR gate.
  /// 2 is for AND gate.
  inline void type(int t) {
    assert(t == 1 || t == 2);
    type_ = t;
  }

  /// @returns Type of this gate. 1 is OR, 2 is AND.
  inline int type() const {
    assert(type_ == 1 || type_ == 2);
    return type_;
  }

  /// Any legal string type of original gate.
  /// This can be helpful for initialization and additional information.
  inline void string_type(std::string type) { string_type_ = type; }

  /// @returns String type of this gate.
  inline std::string string_type() { return string_type_; }

  /// This function is used to initiate this gate with children.
  /// It is assumed that children are passed in ascending order.
  void InitiateWithChild(int child);

  /// Adds a child of this gate.
  /// @returns false If there is a complement of the added child.
  /// @returns true If the addition of this child successful.
  /// @warning This function does not indicate error for future additions.
  bool AddChild(int child);

  bool SwapChild(int existing_child, int new_child);

  /// Adds children of another gate to this gate.
  /// If this gate exists as a child then it is removed from the children.
  /// @param[in] child_gate The gate which children to be added to this gate.
  /// @returns false if the final set is null.
  bool MergeGate(IndexedGate* child_gate);

  inline void EraseAllChildren() { children_.clear(); }

  inline void EraseChild(int child) {
    assert(children_.count(child));
    children_.erase(child);
  }

  inline void Nullify() {
    assert(state_ == "normal");
    state_ = "null";
    children_.clear();
  }

  inline void MakeUnity() {
    assert(state_ == "normal");
    state_ = "unity";
    children_.clear();
  }

  inline int index() { return index_; }

  inline const std::set<int>& children() { return children_; }

  inline std::string state() { return state_; }

 private:
  /// Type of this gate. Only two choices are allowed: OR, AND.
  int type_;

  /// Children of a gate.
  std::set<int> children_;

  /// Index of this gate.
  int index_;

  /// String type of this gate, such as NOR, OR, NAND, XOR.
  std::string string_type_;

  /// Indication if this gate is normal, null, or unity.
  std::string state_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_GATE_H_
