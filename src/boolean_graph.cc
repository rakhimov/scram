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
/// Implementation of indexed nodes, variables, gates, and the Boolean graph.
/// The implementation caters other algorithms like preprocessing.
/// The main goal is
/// to make manipulations and transformations of the graph easier
/// to achieve for graph algorithms.
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
      num_failed_args_(0) {}

bool IGate::AddArg(int arg, const IGatePtr& gate) {
  assert(arg != 0);
  assert(std::abs(arg) == gate->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(args_.empty());
  if (type_ == kXorGate) assert(args_.size() < 2);
  if (args_.count(arg)) return IGate::ProcessDuplicateArg(arg);
  if (args_.count(-arg)) return IGate::ProcessComplementArg(arg);
  args_.insert(arg);
  gate_args_.insert(std::make_pair(arg, gate));
  gate->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
  return true;
}

bool IGate::AddArg(int arg, const VariablePtr& variable) {
  assert(arg != 0);
  assert(std::abs(arg) == variable->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(args_.empty());
  if (type_ == kXorGate) assert(args_.size() < 2);
  if (args_.count(arg)) return IGate::ProcessDuplicateArg(arg);
  if (args_.count(-arg)) return IGate::ProcessComplementArg(arg);
  args_.insert(arg);
  variable_args_.insert(std::make_pair(arg, variable));
  variable->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
  return true;
}

bool IGate::AddArg(int arg, const ConstantPtr& constant) {
  assert(arg != 0);
  assert(std::abs(arg) == constant->index());
  assert(state_ == kNormalState);
  if (type_ == kNotGate || type_ == kNullGate) assert(args_.empty());
  if (type_ == kXorGate) assert(args_.size() < 2);
  if (args_.count(arg)) return IGate::ProcessDuplicateArg(arg);
  if (args_.count(-arg)) return IGate::ProcessComplementArg(arg);
  args_.insert(arg);
  constant_args_.insert(std::make_pair(arg, constant));
  constant->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
  return true;
}

bool IGate::TransferArg(int arg, const IGatePtr& recipient) {
  assert(arg != 0);
  assert(args_.count(arg));
  args_.erase(arg);
  bool ret = true;  // The normal state of the recipient.
  NodePtr node;
  if (gate_args_.count(arg)) {
    node = gate_args_.find(arg)->second;
    ret = recipient->AddArg(arg, gate_args_.find(arg)->second);
    gate_args_.erase(arg);
  } else if (variable_args_.count(arg)) {
    node = variable_args_.find(arg)->second;
    ret = recipient->AddArg(arg, variable_args_.find(arg)->second);
    variable_args_.erase(arg);
  } else {
    assert(constant_args_.count(arg));
    node = constant_args_.find(arg)->second;
    ret = recipient->AddArg(arg, constant_args_.find(arg)->second);
    constant_args_.erase(arg);
  }
  assert(node->parents_.count(Node::index()));
  node->parents_.erase(Node::index());
  return ret;
}

bool IGate::ShareArg(int arg, const IGatePtr& recipient) {
  assert(arg != 0);
  assert(args_.count(arg));
  bool ret = true;  // The normal state of the recipient.
  if (gate_args_.count(arg)) {
    ret = recipient->AddArg(arg, gate_args_.find(arg)->second);
  } else if (variable_args_.count(arg)) {
    ret = recipient->AddArg(arg, variable_args_.find(arg)->second);
  } else {
    assert(constant_args_.count(arg));
    ret = recipient->AddArg(arg, constant_args_.find(arg)->second);
  }
  return ret;
}

void IGate::InvertArgs() {
  std::set<int> args(args_);  // Not to mess the iterator.
  std::set<int>::iterator it;
  for (it = args.begin(); it != args.end(); ++it) {
    IGate::InvertArg(*it);
  }
}

void IGate::InvertArg(int existing_arg) {
  assert(args_.count(existing_arg));
  assert(!args_.count(-existing_arg));
  args_.erase(existing_arg);
  args_.insert(-existing_arg);
  if (gate_args_.count(existing_arg)) {
    IGatePtr arg = gate_args_.find(existing_arg)->second;
    gate_args_.erase(existing_arg);
    gate_args_.insert(std::make_pair(-existing_arg, arg));
  } else if (variable_args_.count(existing_arg)) {
    VariablePtr arg = variable_args_.find(existing_arg)->second;
    variable_args_.erase(existing_arg);
    variable_args_.insert(std::make_pair(-existing_arg, arg));
  } else {
    ConstantPtr arg = constant_args_.find(existing_arg)->second;
    constant_args_.erase(existing_arg);
    constant_args_.insert(std::make_pair(-existing_arg, arg));
  }
}

bool IGate::JoinGate(const IGatePtr& arg_gate) {
  assert(args_.count(arg_gate->index()));  // Positive argument only.
  args_.erase(arg_gate->index());
  gate_args_.erase(arg_gate->index());
  assert(arg_gate->parents_.count(Node::index()));
  arg_gate->parents_.erase(Node::index());

  boost::unordered_map<int, IGatePtr>::const_iterator it_g;
  for (it_g = arg_gate->gate_args_.begin();
       it_g != arg_gate->gate_args_.end(); ++it_g) {
    if (!IGate::AddArg(it_g->first, it_g->second)) return false;
  }
  boost::unordered_map<int, VariablePtr>::const_iterator it_b;
  for (it_b = arg_gate->variable_args_.begin();
       it_b != arg_gate->variable_args_.end(); ++it_b) {
    if (!IGate::AddArg(it_b->first, it_b->second)) return false;
  }
  boost::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = arg_gate->constant_args_.begin();
       it_c != arg_gate->constant_args_.end(); ++it_c) {
    if (!IGate::AddArg(it_c->first, it_c->second)) return false;
  }
  return true;
}

bool IGate::JoinNullGate(int index) {
  assert(index != 0);
  assert(args_.count(index));
  assert(gate_args_.count(index));

  args_.erase(index);
  IGatePtr arg_gate = gate_args_.find(index)->second;
  gate_args_.erase(index);
  arg_gate->parents_.erase(Node::index());

  assert(arg_gate->type_ == kNullGate);
  assert(arg_gate->args_.size() == 1);

  int grandchild = *arg_gate->args_.begin();
  grandchild *= index > 0 ? 1 : -1;  // Carry the parent's sign.

  if (!arg_gate->gate_args_.empty()) {
    return IGate::AddArg(grandchild, arg_gate->gate_args_.begin()->second);
  } else if (!arg_gate->constant_args_.empty()) {
    return IGate::AddArg(grandchild, arg_gate->constant_args_.begin()->second);
  } else {
    assert(!arg_gate->variable_args_.empty());
    return IGate::AddArg(grandchild, arg_gate->variable_args_.begin()->second);
  }
}

bool IGate::ProcessDuplicateArg(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(args_.count(index));
  switch (type_) {
    case kXorGate:
      IGate::Nullify();
      return false;;
    case kAtleastGate:
      // This is a very special handling of K/N duplicates.
      // @(k, [x, x, y_i]) = x & @(k-2, [y_i]) | @(k, [y_i])
      assert(vote_number_ > 1);
      assert(args_.size() > 2);
      type_ = kOrGate;
      std::set<int> to_share(args_);  // Copy before manipulations.
      to_share.erase(index);

      IGatePtr child_and(new IGate(kAndGate));  // May not be needed.
      IGatePtr grand_child;  // Only if child_and is needed.
      if (vote_number_ == 3) {  // Create an OR grand child.
        grand_child = IGatePtr(new IGate(kOrGate));

      } else if (vote_number_ > 3) {  // Create a K/N grand child.
        grand_child = IGatePtr(new IGate(kAtleastGate));
        grand_child->vote_number(vote_number_ - 2);
      }
      if (grand_child) {
        this->AddArg(child_and->index(), child_and);
        this->TransferArg(index, child_and);
        child_and->AddArg(grand_child->index(), grand_child);
        assert(child_and->args().size() == 2);
      }

      IGatePtr child_atleast(new IGate(kAtleastGate));
      child_atleast->vote_number(vote_number_);
      this->AddArg(child_atleast->index(), child_atleast);

      std::set<int>::iterator it;
      for (it = to_share.begin(); it != to_share.end(); ++it) {
        if (grand_child) this->ShareArg(*it, grand_child);
        this->TransferArg(*it, child_atleast);
      }
      assert(args_.size() == 2);
  }
  return true;  // Duplicate arguments are OK in most cases.
}

bool IGate::ProcessComplementArg(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(args_.count(-index));
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
      IGate::EraseArg(-index);
      assert(vote_number_ > 1);
      --vote_number_;
      if (vote_number_ == 1) {
        type_ = kOrGate;
      } else if (vote_number_ == args_.size()) {
        type_ = kAndGate;
      }
      return true;
  }
  return false;  // Becomes constant most of the cases.
}

void IGate::ArgFailed() {
  if (Node::opti_value() == 1) return;
  assert(Node::opti_value() == 0);
  assert(num_failed_args_ < args_.size());
  ++num_failed_args_;
  switch (type_) {
    case kNullGate:
    case kOrGate:
      Node::opti_value(1);
      break;
    case kAndGate:
      if (num_failed_args_ == args_.size()) Node::opti_value(1);
      break;
    case kAtleastGate:
      if (num_failed_args_ == vote_number_) Node::opti_value(1);
      break;
    default:
      assert(false);  // Coherent gates only!
  }
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
  root_ = BooleanGraph::ProcessFormula(root->formula(), ccf, &id_to_index);
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
        parent->AddArg(node->index(), boost::static_pointer_cast<IGate>(node));
      } else {
        parent->AddArg(node->index(),
                       boost::static_pointer_cast<Variable>(node));
      }
    } else {  // Create a new node.
      if (ccf && basic_event->HasCcf()) {  // Create a CCF gate.
        GatePtr ccf_gate = basic_event->ccf_gate();
        IGatePtr new_gate =
            BooleanGraph::ProcessFormula(ccf_gate->formula(), ccf, id_to_index);
        parent->AddArg(new_gate->index(), new_gate);
        id_to_index->insert(std::make_pair(basic_event->id(), new_gate));
      } else {
        basic_events_.push_back(basic_event);
        VariablePtr new_basic(new Variable());  // Sequential indexation.
        assert(basic_events_.size() == new_basic->index());
        parent->AddArg(new_basic->index(), new_basic);
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
      parent->AddArg(node->index(), boost::static_pointer_cast<Constant>(node));
    } else {
      ConstantPtr constant(new Constant(house->state()));
      parent->AddArg(constant->index(), constant);
      id_to_index->insert(std::make_pair(house->id(), constant));
    }
  }

  std::vector<GatePtr>::const_iterator it_g;
  for (it_g = formula->gate_args().begin(); it_g != formula->gate_args().end();
       ++it_g) {
    GatePtr gate = *it_g;
    if (id_to_index->count(gate->id())) {
      NodePtr node = id_to_index->find(gate->id())->second;
      parent->AddArg(node->index(), boost::static_pointer_cast<IGate>(node));
    } else {
      IGatePtr new_gate = BooleanGraph::ProcessFormula(gate->formula(), ccf,
                                                       id_to_index);
      parent->AddArg(new_gate->index(), new_gate);
      id_to_index->insert(std::make_pair(gate->id(), new_gate));
    }
  }

  const std::set<FormulaPtr>& formulas = formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas.begin(); it_f != formulas.end(); ++it_f) {
    IGatePtr new_gate = BooleanGraph::ProcessFormula(*it_f, ccf, id_to_index);
    parent->AddArg(new_gate->index(), new_gate);
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
    std::string state = gate->state() == kNullState ? "false" : "true";
    os << "s(GC" << gate->index() << ") = " << state << std::endl;
    return os;
  }
  std::string formula = "(";  // Beginning string for most formulas.
  std::string op = "";  // Operator of the formula.
  std::string end = ")";  // Closing parentheses for most formulas.
  switch (gate->type()) {  // Determine the beginning string and the operator.
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
      formula = "~(";  // Parentheses are for cases of NOT(NOT Arg).
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
  bool first_arg = true;  // To get the formatting correct.

  typedef boost::shared_ptr<IGate> IGatePtr;
  boost::unordered_map<int, IGatePtr>::const_iterator it_gate;
  for (it_gate = gate->gate_args().begin(); it_gate != gate->gate_args().end();
       ++it_gate) {
    if (first_arg) {
      first_arg = false;
    } else {
      formula += op;
    }
    if (it_gate->first < 0) formula += "~";  // Negation.
    IGatePtr arg_gate = it_gate->second;
    if (arg_gate->state() == kNormalState) {
      formula += "G";
      if (arg_gate->IsModule()) formula += "M";
    } else {
      formula += "GC";
    }
    formula += boost::lexical_cast<std::string>(arg_gate->index());

    os << arg_gate;
  }

  typedef boost::shared_ptr<Variable> VariablePtr;
  boost::unordered_map<int, VariablePtr>::const_iterator it_basic;
  for (it_basic = gate->variable_args().begin();
       it_basic != gate->variable_args().end(); ++it_basic) {
    if (first_arg) {
      first_arg = false;
    } else {
      formula += op;
    }
    if (it_basic->first < 0) formula += "~";  // Negation.
    VariablePtr arg = it_basic->second;
    formula += "B" + boost::lexical_cast<std::string>(arg->index());

    os << arg;
  }

  typedef boost::shared_ptr<Constant> ConstantPtr;
  boost::unordered_map<int, ConstantPtr>::const_iterator it_const;
  for (it_const = gate->constant_args().begin();
       it_const != gate->constant_args().end(); ++it_const) {
    if (first_arg) {
      first_arg = false;
    } else {
      formula += op;
    }
    if (it_const->first < 0) formula += "~";  // Negation.
    ConstantPtr arg = it_const->second;
    formula += "H" + boost::lexical_cast<std::string>(arg->index());

    os << arg;
  }
  std::string signature = "G";
  if (gate->IsModule()) signature += "M";
  os << signature << gate->index() << " := " << formula << end << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const BooleanGraph* ft) {
  os << "BooleanGraph_G" << ft->root()->index() << std::endl;
  os << std::endl;
  os << ft->root();
  return os;
}

}  // namespace scram
