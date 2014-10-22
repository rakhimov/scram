#include "indexed_gate.h"

namespace scram {

IndexedGate::IndexedGate(int index)
    : index_(index),
      type_(-1),
      state_("normal"),
      string_type_("finished") {}

void IndexedGate::InitiateWithChild(int child) {
  assert(child != 0);
  assert(state_ == "normal");
  children_.insert(children_.end(), child);
}

bool IndexedGate::AddChild(int child) {
  assert(type_ == 1 || type_ == 2);  // Type must be already defined.
  assert(child != 0);
  assert(state_ == "normal");
  if (children_.count(-child)) {
    if (type_ == 2) {
      state_ = "null";  // AND gate becomes NULL.
      children_.clear();
      return false;
    }
  }
  children_.insert(child);
  return true;
}

bool IndexedGate::SwapChild(int existing_child, int new_child) {
  assert(children_.count(existing_child));
  children_.erase(existing_child);
  return IndexedGate::AddChild(new_child);
}

/// @returns false if the final set is null.
bool IndexedGate::MergeGate(IndexedGate* child_gate) {
  assert(children_.count(child_gate->index()));
  children_.erase(child_gate->index());
  std::set<int>::const_iterator it;
  for (it = child_gate->children_.begin();
       it != child_gate->children_.end(); ++it) {
    if (!IndexedGate::AddChild(*it)) return false;
  }
  return true;
}

}  // namespace scram
