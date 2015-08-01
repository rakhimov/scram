/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/// @file indexed_fault_tree.cc
/// Implementation of indexed nodes, events, gates, and fault trees.
#include "indexed_fault_tree.h"

#include <utility>

#include <boost/assign.hpp>
#include <boost/pointer_cast.hpp>

#include "event.h"

namespace scram {

int Node::next_index_ = 1e6;  // 1 million basic events per fault tree is crazy!

Node::Node() : index_(next_index_++), opti_value_(0) {
  std::fill(visits_, visits_ + 3, 0);
}

Node::Node(int index) : index_(index), opti_value_(0) {
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
      mark_(false),
      min_time_(0),
      max_time_(0),
      module_(false),
      num_failed_children_(0) {}

bool IGate::AddChild(int child, const IGatePtr& gate) {
  assert(child != 0);
  assert(std::abs(child) == gate->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(children_.empty());
  if (type_ == kXorGate) assert(children_.size() < 2);
  if (children_.count(child)) return IGate::ProcessDuplicateChild(child);
  if (children_.count(-child)) return IGate::ProcessComplementChild(child);
  children_.insert(child);
  gate_children_.insert(std::make_pair(child, gate));
  gate->parents_.insert(this);
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
  basic_event_children_.insert(std::make_pair(child, basic_event));
  basic_event->parents_.insert(this);
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
  constant_children_.insert(std::make_pair(child, constant));
  constant->parents_.insert(this);
  return true;
}

bool IGate::TransferChild(int child, const IGatePtr& recipient) {
  assert(child != 0);
  assert(children_.count(child));
  children_.erase(child);
  bool ret = true;  // The normal state of the recipient.
  NodePtr node;
  if (gate_children_.count(child)) {
    node = gate_children_.find(child)->second;
    ret = recipient->AddChild(child, gate_children_.find(child)->second);
    gate_children_.erase(child);
  } else if (basic_event_children_.count(child)) {
    node = basic_event_children_.find(child)->second;
    ret = recipient->AddChild(child, basic_event_children_.find(child)->second);
    basic_event_children_.erase(child);
  } else {
    assert(constant_children_.count(child));
    node = constant_children_.find(child)->second;
    ret = recipient->AddChild(child, constant_children_.find(child)->second);
    constant_children_.erase(child);
  }
  assert(node->parents_.count(this));
  node->parents_.erase(this);
  return ret;
}

bool IGate::ShareChild(int child, const IGatePtr& recipient) {
  assert(child != 0);
  assert(children_.count(child));
  bool ret = true;  // The normal state of the recipient.
  if (gate_children_.count(child)) {
    ret = recipient->AddChild(child, gate_children_.find(child)->second);
  } else if (basic_event_children_.count(child)) {
    ret = recipient->AddChild(child, basic_event_children_.find(child)->second);
  } else {
    assert(constant_children_.count(child));
    ret = recipient->AddChild(child, constant_children_.find(child)->second);
  }
  return ret;
}

void IGate::InvertChildren() {
  std::set<int> children(children_);  // Not to mess the iterator.
  std::set<int>::iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    IGate::InvertChild(*it);
  }
}

void IGate::InvertChild(int existing_child) {
  assert(children_.count(existing_child));
  children_.erase(existing_child);
  children_.insert(-existing_child);
  if (gate_children_.count(existing_child)) {
    IGatePtr child = gate_children_.find(existing_child)->second;
    gate_children_.erase(existing_child);
    gate_children_.insert(std::make_pair(-existing_child, child));
  } else if (basic_event_children_.count(existing_child)) {
    IBasicEventPtr child = basic_event_children_.find(existing_child)->second;
    basic_event_children_.erase(existing_child);
    basic_event_children_.insert(std::make_pair(-existing_child, child));
  } else {
    ConstantPtr child = constant_children_.find(existing_child)->second;
    constant_children_.erase(existing_child);
    constant_children_.insert(std::make_pair(-existing_child, child));
  }
}

bool IGate::JoinGate(const IGatePtr& child_gate) {
  assert(children_.count(child_gate->index()));  // Positive child only.
  children_.erase(child_gate->index());
  gate_children_.erase(child_gate->index());
  assert(child_gate->parents_.count(this));
  child_gate->parents_.erase(this);
  boost::unordered_map<int, IGatePtr>::const_iterator it_g;
  for (it_g = child_gate->gate_children_.begin();
       it_g != child_gate->gate_children_.end(); ++it_g) {
    if (!IGate::AddChild(it_g->first, it_g->second)) return false;
  }
  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = child_gate->basic_event_children_.begin();
       it_b != child_gate->basic_event_children_.end(); ++it_b) {
    if (!IGate::AddChild(it_b->first, it_b->second)) return false;
  }
  boost::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = child_gate->constant_children_.begin();
       it_c != child_gate->constant_children_.end(); ++it_c) {
    if (!IGate::AddChild(it_c->first, it_c->second)) return false;
  }
  return true;
}

bool IGate::JoinNullGate(int index) {
  assert(index != 0);
  assert(children_.count(index));
  assert(gate_children_.count(index));

  children_.erase(index);
  IGatePtr child_gate = gate_children_.find(index)->second;
  gate_children_.erase(index);
  child_gate->parents_.erase(this);

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

bool IGate::ProcessDuplicateChild(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(type_ != kAtleastGate);  /// @todo Provide the complex logic.
  assert(children_.count(index));
  switch (type_) {
    case kXorGate:
      IGate::Nullify();
      return false;;
  }
  return true;  // Duplicate children are OK in most cases.
}

bool IGate::ProcessComplementChild(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(children_.count(-index));
  switch (type_) {
    case kNorGate:
    case kAndGate:
      IGate::Nullify();
      break;
    case kNandGate:
    case kXorGate:
    case kOrGate:
      IGate::MakeUnity();
      break;
    case kAtleastGate:
      IGate::EraseChild(-index);
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

void IGate::ChildFailed() {
  if (this->opti_value() == 1) return;
  assert(this->opti_value() == 0);
  assert(num_failed_children_ < children_.size());
  ++num_failed_children_;
  switch (type_) {
    case kNullGate:
    case kOrGate:
      this->opti_value(1);
      break;
    case kAndGate:
      if (num_failed_children_ == children_.size()) this->opti_value(1);
      break;
    case kAtleastGate:
      if (num_failed_children_ == vote_number_) this->opti_value(1);
      break;
    default:
      assert(false);  // Coherent gates only!
  }
}

void IGate::ResetChildrenFailure() {
  num_failed_children_ = 0;
}

const std::map<std::string, GateType> IndexedFaultTree::kStringToType_ =
    boost::assign::map_list_of("and", kAndGate) ("or", kOrGate)
                              ("atleast", kAtleastGate) ("xor", kXorGate)
                              ("not", kNotGate) ("nand", kNandGate)
                              ("nor", kNorGate) ("null", kNullGate);

IndexedFaultTree::IndexedFaultTree(const GatePtr& root, bool ccf) 
    : coherent_(true) {
  Node::ResetIndex();
  IBasicEvent::ResetIndex();
  boost::unordered_map<std::string, NodePtr> id_to_index;
  top_event_ = IndexedFaultTree::ProcessFormula(root->formula(), ccf,
                                                &id_to_index);
}

boost::shared_ptr<IGate> IndexedFaultTree::ProcessFormula(
    const FormulaPtr& formula,
    bool ccf,
    boost::unordered_map<std::string, NodePtr>* id_to_index) {
  GateType type = kStringToType_.find(formula->type())->second;
  IGatePtr parent(new IGate(type));
  switch (type) {
    case kNotGate:
    case kNandGate:
    case kNorGate:
    case kXorGate:
      coherent_ = false;
      break;
    case kAtleastGate:
      parent->vote_number(formula->vote_number());
      break;
  }
  std::vector<BasicEventPtr>::const_iterator it_b;
  for (it_b = formula->basic_event_args().begin();
       it_b != formula->basic_event_args().end(); ++it_b) {
    BasicEventPtr basic_event = *it_b;
    if (id_to_index->count(basic_event->id())) {
      NodePtr node = id_to_index->find(basic_event->id())->second;
      if (ccf && basic_event->HasCcf()) {
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
  }

  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  std::vector<HouseEventPtr>::const_iterator it_h;
  for (it_h = formula->house_event_args().begin();
       it_h != formula->house_event_args().end(); ++it_h) {
    HouseEventPtr house = *it_h;
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

  std::vector<GatePtr>::const_iterator it_g;
  for (it_g = formula->gate_args().begin();
       it_g != formula->gate_args().end(); ++it_g) {
    GatePtr gate = *it_g;
    if (id_to_index->count(gate->id())) {
      NodePtr node = id_to_index->find(gate->id())->second;
      parent->AddChild(node->index(),
                       boost::static_pointer_cast<IGate>(node));
    } else {
      IGatePtr new_gate = IndexedFaultTree::ProcessFormula(gate->formula(),
                                                           ccf,
                                                           id_to_index);
      parent->AddChild(new_gate->index(), new_gate);
      id_to_index->insert(std::make_pair(gate->id(), new_gate));
    }
  }

  const std::set<FormulaPtr>& formulas = formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas.begin(); it_f != formulas.end(); ++it_f) {
    IGatePtr new_gate = IndexedFaultTree::ProcessFormula(*it_f, ccf,
                                                         id_to_index);
    parent->AddChild(new_gate->index(), new_gate);
  }
  return parent;
}

}  // namespace scram
