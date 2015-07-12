/// @file indexed_gate.cc
/// Implementation of IndexedGate class for use in IndexedFaultTree.
#include "indexed_gate.h"

namespace scram {

IndexedGate::IndexedGate(int index, const GateType& type)
    : index_(index),
      type_(type),
      state_(kNormalState),
      vote_number_(-1) {
  std::fill(visits_, visits_ + 3, 0);
}

void IndexedGate::InitiateWithChild(int child) {
  assert(child != 0);
  assert(state_ == kNormalState);
  children_.insert(children_.end(), child);
}

bool IndexedGate::AddChild(int child) {
  assert(type_ == kAndGate || type_ == kOrGate);  // Must be normalized.
  assert(child != 0);
  assert(state_ == kNormalState);
  if (children_.count(-child)) {
    state_ = type_ == kAndGate ? kNullState : kUnityState;
    children_.clear();
    return false;
  }
  children_.insert(child);
  return true;
}

bool IndexedGate::SwapChild(int existing_child, int new_child) {
  assert(children_.count(existing_child));
  children_.erase(existing_child);
  return IndexedGate::AddChild(new_child);
}

void IndexedGate::InvertChildren() {
  std::set<int> inverted_children;
  std::set<int>::iterator it;
  for (it = children_.begin(); it != children_.end(); ++it) {
    inverted_children.insert(inverted_children.begin(), -*it);
  }
  children_ = inverted_children;  /// @todo Check swap() for performance.
}

bool IndexedGate::JoinGate(IndexedGate* child_gate) {
  assert(children_.count(child_gate->index()));
  children_.erase(child_gate->index());
  std::set<int>::const_iterator it;
  for (it = child_gate->children_.begin(); it != child_gate->children_.end();
       ++it) {
    if (!IndexedGate::AddChild(*it)) return false;
  }
  return true;
}

}  // namespace scram
