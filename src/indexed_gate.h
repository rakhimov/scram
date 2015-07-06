/// @file indexed_gate.h
/// Representation of a gate with indexed children to use with
/// IndexedFaultTree class.
#ifndef SCRAM_SRC_INDEXED_GATE_H_
#define SCRAM_SRC_INDEXED_GATE_H_

#include <assert.h>

#include <set>
#include <string>

namespace scram {

/// @class IndexedGate
/// This gate is for use in IndexedFaultTree.
/// Initially this gate can represent any type of gate; however,
/// this gate can be only of OR and AND type at the end of all simplifications
/// and processing. This gate class helps to process the fault tree before
/// any complex analysis is done.
class IndexedGate {
 public:
  /// Creates a gate with its index.
  /// @param[in] index An unique positive index of this gate.
  /// @warning The index is not validated upon instantiation.
  explicit IndexedGate(int index);

  /// @returns Type of this gate. 1 is OR, 2 is AND.
  inline int type() const {
    assert(type_ == 1 || type_ == 2);
    return type_;
  }

  /// Sets the gate type information.
  /// 1 is for OR gate.
  /// 2 is for AND gate.
  /// @param[in] t The type for this gate.
  inline void type(int t) {
    assert(t == 1 || t == 2);
    type_ = t;
  }

  /// @returns String type of this gate.
  inline const std::string& string_type() const { return string_type_; }

  /// Sets any legal string type of original gate.
  /// This can be helpful for initialization and additional information.
  /// @param[in] type The string (OR, AND, ...) type for this gate.
  inline void string_type(const std::string& type) { string_type_ = type; }

  /// @returns Vote number.
  inline int vote_number() const { return vote_number_; }

  /// Sets the vote number for this gate. The function does not check if
  /// the gate type is ATLEAST.
  /// @param[in] number The vote number of ATLEAST gate.
  inline void vote_number(int number) { vote_number_ = number; }

  /// @returns The index of this gate.
  inline int index() const { return index_; }

  /// Sets the index of this gate.
  /// @param[in] index Positive index of this gate.
  inline void index(int index) {
    assert(index > 0);
    index_ = index;
  }

  /// @returns children of this gate.
  inline const std::set<int>& children() const { return children_; }

  /// Directly assigns children for this gate.
  /// @param[in] children A new set of children for this gate.
  inline void children(const std::set<int>& children) { children_ = children; }

  /// @returns The state of this gate, which is either "null", or "unity", or
  ///          "normal" by default.
  inline const std::string& state() const { return state_; }

  /// @returns parents of this gate.
  inline const std::set<int>& parents() { return parents_; }

  /// This function is used to initiate this gate with children.
  /// It is assumed that children are passed in ascending order from another
  /// children set.
  /// @param[in] child A positive or negative index of a child.
  void InitiateWithChild(int child);

  /// Adds a child to this gate. Before adding the child, the existing
  /// children are checked for complements. If there is a complement,
  /// the gate changes its state and clears its children. This functionality
  /// only works with OR and AND gates.
  /// @param[in] child A positive or negative index of a child.
  /// @returns false If there is a complement of the child being added.
  /// @returns true If the addition of this child is successful.
  /// @warning This function does not indicate error for future additions in
  ///          case the state is nulled or becomes unity.
  bool AddChild(int child);

  /// Swaps an existing child to a new child. Mainly used for
  /// changing the logic of this gate or complementing the child.
  /// @param[in] existing_child An existing child to get swapped.
  /// @param[in] new_child A new child.
  /// @warning If there is an iterator for the children set, then
  ///          it may become unusable because the children set is manipulated.
  bool SwapChild(int existing_child, int new_child);

  /// Makes all children complement of themselves.
  /// This is a helper function to propagate a complement gate and apply
  /// De Morgan's Law.
  void InvertChildren();

  /// Adds children of another gate to this gate.
  /// If this gate exists as a child then it is removed from the children.
  /// @param[in] child_gate The gate which children to be added to this gate.
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  bool MergeGate(IndexedGate* child_gate);

  /// Clears all the children of this gate.
  inline void EraseAllChildren() { children_.clear(); }

  /// Removes a child from the children container. The passed child index
  /// must be in this gate's children container and initialized.
  /// @param[in] child The positive or negative index of the existing child.
  inline void EraseChild(int child) {
    assert(children_.count(child));
    children_.erase(child);
  }

  /// Sets the state of this gate to null and clears all its children.
  inline void Nullify() {
    assert(state_ == "normal");
    state_ = "null";
    children_.clear();
  }

  /// Sets the state of this gate to unity and clears all its children.
  inline void MakeUnity() {
    assert(state_ == "normal");
    state_ = "unity";
    children_.clear();
  }

  /// Adds a parent of this gate.
  /// @param[in] index Positive index of the parent.
  inline void AddParent(int index) {
    assert(index > 0);
    parents_.insert(index);
  }

  /// Removes a parent of this gate.
  /// @param[in] index Positive index of the existing parent.
  inline void EraseParent(int index) {
    assert(index > 0);
    assert(parents_.count(index));
    parents_.erase(index);
  }

  /// Registers the visit time for this gate upon tree traversal.
  /// This information can be used to detect dependencies.
  /// @param[in] time The visit time of this gate.
  /// @returns true If this gate was previously visited.
  /// @returns false If this is visited and re-visited only once.
  bool Visit(int time) {
    assert(time > 0);
    if (!visits_[0]) {
      visits_[0] = time;
    } else if (!visits_[1]) {
      visits_[1] = time;
    } else {
      visits_[2] = time;
      return true;
    }
    return false;
  }

  /// @returns The time when this gate was first encountered or entered.
  inline int EnterTime() const { return visits_[0]; }

  /// @returns The exit time upon traversal of the tree.
  inline int ExitTime() const { return visits_[1]; }

  /// @returns The last time this gate was visited.
  inline int LastVisit() const { return visits_[2] ? visits_[2] : visits_[1]; }

  /// @returns false if this gate was only visited once upon tree traversal.
  /// @returns true if this gate was revisited at one more time.
  inline bool Revisited() const { return visits_[2] ? true : false; }

 private:
  int type_;  ///< Type of this gate. Only two choices are allowed: OR, AND.
  std::set<int> children_;  ///< Children of a gate.
  int index_;  ///< Index of this gate.
  std::string string_type_;  ///< String type of this gate, such as NOR, OR.
  std::string state_;  ///< Indication if this gate is normal, null, or unity.
  int vote_number_;  ///< Vote number for atleast gate.
  std::set<int> parents_;  ///< Parents of this gate.
  /// This is a traversal array containing first, second, and last visits.
  int visits_[3];
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_GATE_H_
