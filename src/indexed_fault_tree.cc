/// @file indexed_fault_tree.cc
/// Implementation of indexed nodes, events, gates, and fault trees.
#include "indexed_fault_tree.h"

#include <utility>

#include <boost/assign.hpp>
#include <boost/pointer_cast.hpp>

#include "event.h"

namespace scram {

int Node::next_index_ = 1e6;  // 1 million basic events per fault tree is crazy!

Node::Node() : index_(next_index_++) {
  std::fill(visits_, visits_ + 3, 0);
}

Node::Node(int index) : index_(index) {
  std::fill(visits_, visits_ + 3, 0);
}

Node::~Node() {}  // Empty body for pure virtual destructor.

Constant::Constant(bool state) : Node(), state_(state) {}

int IBasicEvent::next_basic_event_ = 1;

IBasicEvent::IBasicEvent() : Node(next_basic_event_++) {}

IGate::IGate(const GateType& type)
    : Node(),
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

bool IGate::AddChild(int child, const IGatePtr& gate) {
  assert(child != 0);
  assert(std::abs(child) == gate->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(children_.empty());
  if (type_ == kXorGate) assert(children_.size() < 2);
  if (children_.count(-child)) return IGate::ProcessComplementChild(child);
  children_.insert(child);
  gate_children_.insert(std::make_pair(gate->index(), gate));
  return true;
}

bool IGate::AddChild(int child, const IBasicEventPtr& basic_event) {
  assert(child != 0);
  assert(std::abs(child) == basic_event->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(children_.empty());
  if (type_ == kXorGate) assert(children_.size() < 2);
  if (children_.count(-child)) return IGate::ProcessComplementChild(child);
  children_.insert(child);
  basic_event_children_.insert(std::make_pair(basic_event->index(),
                                              basic_event));
  return true;
}

bool IGate::AddChild(int child, const ConstantPtr& constant) {
  assert(child != 0);
  assert(std::abs(child) == constant->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(children_.empty());
  if (type_ == kXorGate) assert(children_.size() < 2);
  if (children_.count(-child)) return IGate::ProcessComplementChild(child);
  children_.insert(child);
  constant_children_.insert(std::make_pair(constant->index(), constant));
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
  children_ = inverted_children;
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

bool IGate::JoinNullGate(int index) {
  assert(index != 0);
  assert(children_.count(index));
  assert(gate_children_.count(index));

  children_.erase(index);
  IGatePtr child_gate = gate_children_.find(std::abs(index))->second;
  gate_children_.erase(std::abs(index));

  assert(child_gate->type_ == kNullGate);
  assert(child_gate->children_.size() == 1);

  int grandchild = *child_gate->children_.begin();
  grandchild *= index > 0 ? 1 : -1;  // Carry the parent's sign.

  if (!child_gate->gate_children_.empty()) {
    return IGate::AddChild(grandchild,
                           child_gate->gate_children_.begin()->second);
  } else if (!child_gate->constant_children_.empty()) {
    return IGate::AddChild(grandchild,
                           child_gate->constant_children_.begin()->second);
  } else {
    assert(!child_gate->basic_event_children_.empty());
    return IGate::AddChild(grandchild,
                           child_gate->basic_event_children_.begin()->second);
  }
}

bool IGate::ProcessComplementChild(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(children_.count(-index));
  switch (type_) {
    case kNorGate:
    case kAndGate:
      state_ = kNullState;
      children_.clear();
      break;
    case kNandGate:
    case kXorGate:
    case kOrGate:
      state_ = kUnityState;
      children_.clear();
      break;
    case kAtleastGate:
      children_.erase(-index);
      assert(vote_number_ > 1);
      --vote_number_;
      if (vote_number_ == 1) {
        type_ = kOrGate;
      } else if (vote_number_ == children_.size()) {
        type_ = kAndGate;
      }
      return true;
  }
  return false;  // Becomes constant most of the cases.
}

const std::map<std::string, GateType> IndexedFaultTree::kStringToType_ =
    boost::assign::map_list_of("and", kAndGate) ("or", kOrGate)
                              ("atleast", kAtleastGate) ("xor", kXorGate)
                              ("not", kNotGate) ("nand", kNandGate)
                              ("nor", kNorGate) ("null", kNullGate);

IndexedFaultTree::IndexedFaultTree(const GatePtr& root, bool ccf)
    : kGateIndex_(1e6) {
  boost::unordered_map<std::string, NodePtr> id_to_index;
  IGatePtr top_event = IndexedFaultTree::ProcessFormula(root->formula(),
                                                        ccf,
                                                        &id_to_index);
  IndexedFaultTree::AddGate(top_event);  /// @todo Remove.
  top_event_index_ = top_event->index();
}

IndexedFaultTree::IndexedFaultTree(int top_event_id)
    : top_event_index_(top_event_id),
      kGateIndex_(top_event_id) {}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter.begin(); it != int_to_inter.end(); ++it) {
    IGatePtr gate =
        IndexedFaultTree::ProcessFormula(it->second->formula(),
                                         ccf_basic_to_gates, all_to_int);
    gate->index(it->first);
    IndexedFaultTree::AddGate(gate);
  }
}

boost::shared_ptr<IGate> IndexedFaultTree::ProcessFormula(
    const FormulaPtr& formula,
    bool ccf,
    boost::unordered_map<std::string, NodePtr>* id_to_index) {
  GateType type = kStringToType_.find(formula->type())->second;
  IGatePtr parent(new IGate(type));
  if (type == kAtleastGate) parent->vote_number(formula->vote_number());

  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;

  const std::map<std::string, EventPtr>& children = formula->event_args();
  std::map<std::string, EventPtr>::const_iterator it_children;
  for (it_children = children.begin(); it_children != children.end();
       ++it_children) {
    EventPtr event = it_children->second;
    if (BasicEventPtr basic_event =
        boost::dynamic_pointer_cast<BasicEvent>(event)) {
      if (id_to_index->count(basic_event->id())) {
        NodePtr node = id_to_index->find(basic_event->id())->second;
        if (ccf) {
          parent->AddChild(node->index(),
                           boost::static_pointer_cast<IGate>(node));
        } else {
          parent->AddChild(node->index(),
                           boost::static_pointer_cast<IBasicEvent>(node));
        }
      } else {
        if (ccf && basic_event->HasCcf()) {
          GatePtr ccf_gate = basic_event->ccf_gate();
          IGatePtr new_gate = IndexedFaultTree::ProcessFormula(
              ccf_gate->formula(),
              ccf,
              id_to_index);
          IndexedFaultTree::AddGate(new_gate);  /// @todo Remove.
          parent->AddChild(new_gate->index(), new_gate);
          id_to_index->insert(std::make_pair(basic_event->id(), new_gate));
        } else {
          basic_events_.push_back(basic_event);
          IBasicEventPtr new_basic(new IBasicEvent());
          assert(basic_events_.size() == new_basic->index());
          parent->AddChild(new_basic->index(), new_basic);
          id_to_index->insert(std::make_pair(basic_event->id(), new_basic));
        }
      }
    } else if (GatePtr gate = boost::dynamic_pointer_cast<Gate>(event)) {
      if (id_to_index->count(gate->id())) {
        NodePtr node = id_to_index->find(gate->id())->second;
        parent->AddChild(node->index(),
                         boost::static_pointer_cast<IGate>(node));
      } else {
        IGatePtr new_gate = IndexedFaultTree::ProcessFormula(gate->formula(),
                                                             ccf,
                                                             id_to_index);
        IndexedFaultTree::AddGate(new_gate);  /// @todo Remove.
        parent->AddChild(new_gate->index(), new_gate);
        id_to_index->insert(std::make_pair(gate->id(), new_gate));
      }

    } else {
      HouseEventPtr house = boost::dynamic_pointer_cast<HouseEvent>(event);
      assert(house);
      if (id_to_index->count(house->id())) {
        NodePtr node = id_to_index->find(house->id())->second;
        parent->AddChild(node->index(),
                         boost::static_pointer_cast<Constant>(node));
      } else {
        ConstantPtr constant(new Constant(house->state()));
        parent->AddChild(constant->index(), constant);
        id_to_index->insert(std::make_pair(house->id(), constant));
      }
    }
  }
  const std::set<FormulaPtr>& formulas = formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas.begin(); it_f != formulas.end(); ++it_f) {
    IGatePtr new_gate = IndexedFaultTree::ProcessFormula(*it_f, ccf,
                                                         id_to_index);
    parent->AddChild(new_gate->index(), new_gate);
    IndexedFaultTree::AddGate(new_gate);  /// @todo Remove.
  }
  return parent;
}

boost::shared_ptr<IGate> IndexedFaultTree::ProcessFormula(
    const FormulaPtr& formula,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  GateType type = kStringToType_.find(formula->type())->second;
  IGatePtr gate(new IGate(type));
  if (type == kAtleastGate) gate->vote_number(formula->vote_number());
  assert(!indexed_gates_.count(gate->index()));

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
    IGatePtr child_gate = IndexedFaultTree::ProcessFormula(*it_f,
                                                           ccf_basic_to_gates,
                                                           all_to_int);
    IndexedFaultTree::AddGate(child_gate);
    gate->InitiateWithChild(child_gate->index());
  }
  return gate;
}

}  // namespace scram
