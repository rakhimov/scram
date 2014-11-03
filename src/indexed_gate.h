/// @file indexed_gate.h
/// Representation of a gate with indexed children to use with
/// IndexedFaultTree class.
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
  /// @param[in] t The type for this gate.
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

  /// @returns Vote number.
  inline int vote_number() { return vote_number_; }

  /// Sets the vote number.
  inline void vote_number(int number) { vote_number_ = number; }

  /// This function is used to initiate this gate with children.
  /// It is assumed that children are passed in ascending order.
  void InitiateWithChild(int child);

  /// This function is used to initiate this gate with children.
  void InitiateWithChildren(const std::set<int>& children) {
    children_ = children;
  }

  /// Adds a child of this gate.
  /// @returns false If there is a complement of the added child.
  /// @returns true If the addition of this child successful.
  /// @warning This function does not indicate error for future additions.
  ///          This logic is done because of the possible application of
  ///          De Morgan's law.
  bool AddChild(int child);

  /// Swaps an existing child to a new child. Mainly used for
  /// changing the logic of this gate or complementing the child.
  /// @param[in] existing_child An existing child to get swapped.
  /// @param[in] new_child A new child.
  /// @warning If there is an iterator for the children set, then
  ///          it may become unusable.
  bool SwapChild(int existing_child, int new_child);

  /// Makes all children complement of themselves.
  /// This is a helper function to propagate the complement gate and apply
  /// De Morgan's Law.
  void InvertChildren();

  /// Adds children of another gate to this gate.
  /// If this gate exists as a child then it is removed from the children.
  /// @param[in] child_gate The gate which children to be added to this gate.
  /// @returns false if the final set is null.
  bool MergeGate(IndexedGate* child_gate);

  /// Clears all the children of this gate.
  inline void EraseAllChildren() { children_.clear(); }

  /// Removes a child from the children container.
  /// @param[in] child The positive or negative index of the existing child.
  inline void EraseChild(int child) {
    assert(children_.count(child));
    children_.erase(child);
  }

  /// Sets the state of this gate to null.
  inline void Nullify() {
    assert(state_ == "normal");
    state_ = "null";
    children_.clear();
    num_primary_ = 0;
  }

  /// Sets the state of this gate to unity.
  inline void MakeUnity() {
    assert(state_ == "normal");
    state_ = "unity";
    children_.clear();
    num_primary_ = 0;
  }

  /// Sets the index of this gate.
  inline void index(int index) { index_ = index; }

  /// @returns The index of this gate.
  inline int index() const { return index_; }

  /// Directly assigns children for this gate.
  /// @param[in] children A new set of children for this gate.
  inline void children(const std::set<int>& children) { children_ = children; }

  /// @returns children of this gate.
  inline const std::set<int>& children() const { return children_; }

  /// @returns The state of this gate, which is either "null", or "unity", or
  ///          "normal" by default.
  inline std::string state() const { return state_; }

  /// Sets the discrimination index for primary events.
  /// @param[in] top_gate The index below which consider primary events.
  static void top_index(int top_gate) { top_index_ = top_gate; }

  /// @returns The number of primary events.
  inline int num_primary() const { return num_primary_; }

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

  /// Vote number for atleast gate.
  int vote_number_;

  /// The index below which the event is considered primary.
  static int top_index_;

  /// The number of primary events.
  int num_primary_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_GATE_H_
