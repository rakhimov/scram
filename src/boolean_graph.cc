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
/// @file boolean_graph.cc
/// Implementation of indexed nodes, events, gates, and the Boolean graph.
#include "boolean_graph.h"

#include <utility>

#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
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

int Variable::next_variable_ = 1;

Variable::Variable() : Node(next_variable_++) {}

IGate::IGate(const Operator& type)
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
  gate->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
  return true;
}

bool IGate::AddChild(int child, const VariablePtr& variable) {
  assert(child != 0);
  assert(std::abs(child) == variable->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(children_.empty());
  if (type_ == kXorGate) assert(children_.size() < 2);
  if (children_.count(child)) return IGate::ProcessDuplicateChild(child);
  if (children_.count(-child)) return IGate::ProcessComplementChild(child);
  children_.insert(child);
  variable_children_.insert(std::make_pair(child, variable));
  variable->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
  return true;
}

bool IGate::AddChild(int child, const ConstantPtr& constant) {
  assert(child != 0);
  assert(std::abs(child) == constant->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(children_.empty());
  if (type_ == kXorGate) assert(children_.size() < 2);
  if (children_.count(child)) return IGate::ProcessDuplicateChild(child);
  if (children_.count(-child)) return IGate::ProcessComplementChild(child);
  children_.insert(child);
  constant_children_.insert(std::make_pair(child, constant));
  constant->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
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
  } else if (variable_children_.count(child)) {
    node = variable_children_.find(child)->second;
    ret = recipient->AddChild(child, variable_children_.find(child)->second);
    variable_children_.erase(child);
  } else {
    assert(constant_children_.count(child));
    node = constant_children_.find(child)->second;
    ret = recipient->AddChild(child, constant_children_.find(child)->second);
    constant_children_.erase(child);
  }
  assert(node->parents_.count(Node::index()));
  node->parents_.erase(Node::index());
  return ret;
}

bool IGate::ShareChild(int child, const IGatePtr& recipient) {
  assert(child != 0);
  assert(children_.count(child));
  bool ret = true;  // The normal state of the recipient.
  if (gate_children_.count(child)) {
    ret = recipient->AddChild(child, gate_children_.find(child)->second);
  } else if (variable_children_.count(child)) {
    ret = recipient->AddChild(child, variable_children_.find(child)->second);
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
  assert(!children_.count(-existing_child));
  children_.erase(existing_child);
  children_.insert(-existing_child);
  if (gate_children_.count(existing_child)) {
    IGatePtr child = gate_children_.find(existing_child)->second;
    gate_children_.erase(existing_child);
    gate_children_.insert(std::make_pair(-existing_child, child));
  } else if (variable_children_.count(existing_child)) {
    VariablePtr child = variable_children_.find(existing_child)->second;
    variable_children_.erase(existing_child);
    variable_children_.insert(std::make_pair(-existing_child, child));
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
  assert(child_gate->parents_.count(Node::index()));
  child_gate->parents_.erase(Node::index());

  boost::unordered_map<int, IGatePtr>::const_iterator it_g;
  for (it_g = child_gate->gate_children_.begin();
       it_g != child_gate->gate_children_.end(); ++it_g) {
    if (!IGate::AddChild(it_g->first, it_g->second)) return false;
  }
  boost::unordered_map<int, VariablePtr>::const_iterator it_b;
  for (it_b = child_gate->variable_children_.begin();
       it_b != child_gate->variable_children_.end(); ++it_b) {
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
  child_gate->parents_.erase(Node::index());

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
    assert(!child_gate->variable_children_.empty());
    return IGate::AddChild(grandchild,
                           child_gate->variable_children_.begin()->second);
  }
}

bool IGate::ProcessDuplicateChild(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(children_.count(index));
  switch (type_) {
    case kXorGate:
      IGate::Nullify();
      return false;;
    case kAtleastGate:
      // This is a very special handling of K/N duplicates.
      // @(k, [x, x, y_i]) = x & @(k-2, [y_i]) | @(k, [y_i])
      assert(vote_number_ > 1);
      assert(children_.size() > 2);
      type_ = kOrGate;
      std::set<int> to_share(children_);  // Copy before manipulations.
      to_share.erase(index);

      IGatePtr child_and(new IGate(kAndGate));  // May not be needed.
      IGatePtr grand_child;  // Only if child_and is needed.
      if (vote_number_ == 3) {  // Create an OR grandchild.
        grand_child = IGatePtr(new IGate(kOrGate));

      } else if (vote_number_ > 3) {  // Create a K/N grandchild.
        grand_child = IGatePtr(new IGate(kAtleastGate));
        grand_child->vote_number(vote_number_ - 2);
      }
      if (grand_child) {
        this->AddChild(child_and->index(), child_and);
        this->TransferChild(index, child_and);
        child_and->AddChild(grand_child->index(), grand_child);
        assert(child_and->children().size() == 2);
      }

      IGatePtr child_atleast(new IGate(kAtleastGate));
      child_atleast->vote_number(vote_number_);
      this->AddChild(child_atleast->index(), child_atleast);

      std::set<int>::iterator it;
      for (it = to_share.begin(); it != to_share.end(); ++it) {
        if (grand_child) this->ShareChild(*it, grand_child);
        this->TransferChild(*it, child_atleast);
      }
      assert(children_.size() == 2);
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
  if (Node::opti_value() == 1) return;
  assert(Node::opti_value() == 0);
  assert(num_failed_children_ < children_.size());
  ++num_failed_children_;
  switch (type_) {
    case kNullGate:
    case kOrGate:
      Node::opti_value(1);
      break;
    case kAndGate:
      if (num_failed_children_ == children_.size()) Node::opti_value(1);
      break;
    case kAtleastGate:
      if (num_failed_children_ == vote_number_) Node::opti_value(1);
      break;
    default:
      assert(false);  // Coherent gates only!
  }
}

void IGate::ResetChildrenFailure() {
  num_failed_children_ = 0;
}

const std::map<std::string, Operator> BooleanGraph::kStringToType_ =
    boost::assign::map_list_of("and", kAndGate) ("or", kOrGate)
                              ("atleast", kAtleastGate) ("xor", kXorGate)
                              ("not", kNotGate) ("nand", kNandGate)
                              ("nor", kNorGate) ("null", kNullGate);

BooleanGraph::BooleanGraph(const GatePtr& root, bool ccf)
    : coherent_(true),
      constants_(false),
      normal_(true) {
  Node::ResetIndex();
  Variable::ResetIndex();
  boost::unordered_map<std::string, NodePtr> id_to_index;
  top_event_ = BooleanGraph::ProcessFormula(root->formula(), ccf, &id_to_index);
}

boost::shared_ptr<IGate> BooleanGraph::ProcessFormula(
    const FormulaPtr& formula,
    bool ccf,
    boost::unordered_map<std::string, NodePtr>* id_to_index) {
  Operator type = kStringToType_.find(formula->type())->second;
  IGatePtr parent(new IGate(type));

  if (normal_ && type != kOrGate && type != kAndGate) normal_ = false;

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
    if (id_to_index->count(basic_event->id())) {  // Node already exists.
      NodePtr node = id_to_index->find(basic_event->id())->second;
      if (ccf && basic_event->HasCcf()) {  // Replace with a CCF gate.
        parent->AddChild(node->index(),
                         boost::static_pointer_cast<IGate>(node));
      } else {
        parent->AddChild(node->index(),
                         boost::static_pointer_cast<Variable>(node));
      }
    } else {  // Create a new node.
      if (ccf && basic_event->HasCcf()) {  // Create a CCF gate.
        GatePtr ccf_gate = basic_event->ccf_gate();
        IGatePtr new_gate =
            BooleanGraph::ProcessFormula(ccf_gate->formula(), ccf, id_to_index);
        parent->AddChild(new_gate->index(), new_gate);
        id_to_index->insert(std::make_pair(basic_event->id(), new_gate));
      } else {
        basic_events_.push_back(basic_event);
        VariablePtr new_basic(new Variable());  // Sequential indexation.
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
    constants_ = true;

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
  for (it_g = formula->gate_args().begin(); it_g != formula->gate_args().end();
       ++it_g) {
    GatePtr gate = *it_g;
    if (id_to_index->count(gate->id())) {
      NodePtr node = id_to_index->find(gate->id())->second;
      parent->AddChild(node->index(),
                       boost::static_pointer_cast<IGate>(node));
    } else {
      IGatePtr new_gate = BooleanGraph::ProcessFormula(gate->formula(), ccf,
                                                       id_to_index);
      parent->AddChild(new_gate->index(), new_gate);
      id_to_index->insert(std::make_pair(gate->id(), new_gate));
    }
  }

  const std::set<FormulaPtr>& formulas = formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas.begin(); it_f != formulas.end(); ++it_f) {
    IGatePtr new_gate = BooleanGraph::ProcessFormula(*it_f, ccf, id_to_index);
    parent->AddChild(new_gate->index(), new_gate);
  }
  return parent;
}

std::ostream& operator<<(std::ostream& os,
                         const boost::shared_ptr<Constant>& constant) {
  if (constant->Visited()) return os;
  constant->Visit(1);
  std::string state = constant->state() ? "true" : "false";
  os << "s(H" << constant->index() << ") = " << state << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const boost::shared_ptr<Variable>& variable) {
  if (variable->Visited()) return os;
  variable->Visit(1);
  os << "p(B" << variable->index() << ") = " << 1 << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const boost::shared_ptr<IGate>& gate) {
  if (gate->Visited()) return os;
  gate->Visit(1);
  if (gate->state() != kNormalState) {
    std::string state = gate->state() == kNullState ? "false" : "true" ;
    os << "s(GC" << gate->index() << ") = " << state << std::endl;
    return os;
  }
  std::string formula = "(";  // Begining string for most formulas.
  std::string op = "";  // Operator of the formula.
  std::string end = ")";  // Closing parentheses for most formulas.
  switch (gate->type()) {  // Determine the begining string and the operator.
    case kNandGate:
      formula = "~(";  // Fall-through to AND gate.
    case kAndGate:
      op = " & ";
      break;
    case kNorGate:
      formula = "~(";  // Fall-through to OR gate.
    case kOrGate:
      op = " | ";
      break;
    case kXorGate:
      op = " ^ ";
      break;
    case kNotGate:
      formula = "~(";  // Parentheses are for cases of NOT(NOT Child).
      break;
    case kNullGate:
      formula = "";  // No need for the parentheses.
      end = "";
      break;
    case kAtleastGate:
      formula = "@(" + boost::lexical_cast<std::string>(gate->vote_number());
      formula += ", [";
      op = ", ";
      end = "])";
      break;
  }
  bool first_child = true;  // To get the formatting correct.

  typedef boost::shared_ptr<IGate> IGatePtr;
  boost::unordered_map<int, IGatePtr>::const_iterator it_gate;
  for (it_gate = gate->gate_children().begin();
       it_gate != gate->gate_children().end(); ++it_gate) {
    if (first_child) {
      first_child = false;
    } else {
      formula += op;
    }
    if (it_gate->first < 0) formula += "~";  // Negation.
    IGatePtr child_gate = it_gate->second;
    if (child_gate->state() == kNormalState) {
      formula += "G";
      if (child_gate->IsModule()) formula += "M";
    } else {
      formula += "GC";
    }
    formula += boost::lexical_cast<std::string>(child_gate->index());

    os << child_gate;
  }

  typedef boost::shared_ptr<Variable> VariablePtr;
  boost::unordered_map<int, VariablePtr>::const_iterator it_basic;
  for (it_basic = gate->variable_children().begin();
       it_basic != gate->variable_children().end(); ++it_basic) {
    if (first_child) {
      first_child = false;
    } else {
      formula += op;
    }
    if (it_basic->first < 0) formula += "~";  // Negation.
    VariablePtr child = it_basic->second;
    formula += "B" + boost::lexical_cast<std::string>(child->index());

    os << child;
  }

  typedef boost::shared_ptr<Constant> ConstantPtr;
  boost::unordered_map<int, ConstantPtr>::const_iterator it_const;
  for (it_const = gate->constant_children().begin();
       it_const != gate->constant_children().end(); ++it_const) {
    if (first_child) {
      first_child = false;
    } else {
      formula += op;
    }
    if (it_const->first < 0) formula += "~";  // Negation.
    ConstantPtr child = it_const->second;
    formula += "H" + boost::lexical_cast<std::string>(child->index());

    os << child;
  }
  std::string signature = "G";
  if (gate->IsModule()) signature += "M";
  os << signature << gate->index() << " := " << formula << end << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const BooleanGraph* ft) {
  os << "BooleanGraph_G" << ft->top_event()->index() << std::endl;
  os << std::endl;
  os << ft->top_event();
  return os;
}

}  // namespace scram
