/// @file indexed_fault_tree.cc
/// Implementation of indexed nodes, events, gates, and fault trees.
#include "indexed_fault_tree.h"

#include <utility>

#include <boost/assign.hpp>

#include "event.h"

namespace scram {

int Node::next_index_ = 1;

Node::Node() : index_(next_index_++) {
  std::fill(visits_, visits_ + 3, 0);
}

Node::Node(int index) : index_(index) {
  std::fill(visits_, visits_ + 3, 0);
}

Node::~Node() {}  // Empty body for pure virtual destructor.

Constant::Constant(bool state) : Node(), state_(state) {}

IBasicEvent::IBasicEvent() : Node() {}

IGate::IGate(int index, const GateType& type)
    : Node(index),
      type_(type),
      state_(kNormalState),
      vote_number_(-1),
      module_(false) {}

void IGate::InitiateWithChild(int child) {
  assert(child != 0);
  assert(state_ == kNormalState);
  children_.insert(children_.end(), child);
}

bool IGate::AddChild(int child) {
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

bool IGate::SwapChild(int existing_child, int new_child) {
  assert(children_.count(existing_child));
  children_.erase(existing_child);
  return IGate::AddChild(new_child);
}

void IGate::InvertChildren() {
  std::set<int> inverted_children;
  std::set<int>::iterator it;
  for (it = children_.begin(); it != children_.end(); ++it) {
    inverted_children.insert(inverted_children.begin(), -*it);
  }
  children_ = inverted_children;  /// @todo Check swap() for performance.
}

void IGate::InvertChild(int existing_child) {
  assert(children_.count(existing_child));
  children_.erase(existing_child);
  children_.insert(-existing_child);
}

bool IGate::JoinGate(IGate* child_gate) {
  assert(children_.count(child_gate->index()));
  children_.erase(child_gate->index());
  std::set<int>::const_iterator it;
  for (it = child_gate->children_.begin(); it != child_gate->children_.end();
       ++it) {
    if (!IGate::AddChild(*it)) return false;
  }
  return true;
}

const std::map<std::string, GateType> IndexedFaultTree::kStringToType_ =
    boost::assign::map_list_of("and", kAndGate) ("or", kOrGate)
                              ("atleast", kAtleastGate) ("xor", kXorGate)
                              ("not", kNotGate) ("nand", kNandGate)
                              ("nor", kNorGate) ("null", kNullGate);

IndexedFaultTree::IndexedFaultTree(int top_event_id)
    : top_event_index_(top_event_id),
      kGateIndex_(top_event_id),
      new_gate_index_(0) {}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  // Assume that new ccf_gates are not re-added into general index container.
  new_gate_index_ = all_to_int.size() + ccf_basic_to_gates.size() + 1;

  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter.begin(); it != int_to_inter.end(); ++it) {
    IndexedFaultTree::ProcessFormula(it->first, it->second->formula(),
                                     ccf_basic_to_gates, all_to_int);
  }
}

void IndexedFaultTree::ProcessFormula(
    int index,
    const FormulaPtr& formula,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  assert(!indexed_gates_.count(index));
  GateType type = kStringToType_.find(formula->type())->second;
  IGatePtr gate(new IGate(index, type));
  if (type == kAtleastGate) gate->vote_number(formula->vote_number());

  typedef boost::shared_ptr<Event> EventPtr;

  const std::map<std::string, EventPtr>* children = &formula->event_args();
  std::map<std::string, EventPtr>::const_iterator it_children;
  for (it_children = children->begin(); it_children != children->end();
       ++it_children) {
    int child_index = all_to_int.find(it_children->first)->second;
    // Replace CCF basic events with the corresponding events.
    if (ccf_basic_to_gates.count(it_children->first))
      child_index = ccf_basic_to_gates.find(it_children->first)->second;
    gate->InitiateWithChild(child_index);
  }
  const std::set<FormulaPtr>* formulas = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas->begin(); it_f != formulas->end(); ++it_f) {
    int child_index = ++new_gate_index_;
    IndexedFaultTree::ProcessFormula(child_index, *it_f, ccf_basic_to_gates,
                                     all_to_int);
    gate->InitiateWithChild(child_index);
  }
  IndexedFaultTree::AddGate(gate);
}

}  // namespace scram
