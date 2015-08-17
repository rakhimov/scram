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

#include "event.h"
#include "logger.h"

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

std::shared_ptr<IGate> IGate::Clone() {
  IGatePtr clone(new IGate(type_));  // The same type.
  clone->vote_number_ = vote_number_;  // Copy vote number in case it is K/N.
  // Getting arguments copied.
  clone->args_ = args_;
  clone->gate_args_ = gate_args_;
  clone->variable_args_ = variable_args_;
  clone->constant_args_ = constant_args_;
  // Introducing the new parent to the args.
  std::unordered_map<int, IGatePtr>::const_iterator it_g;
  for (it_g = gate_args_.begin(); it_g != gate_args_.end(); ++it_g) {
    it_g->second->parents_.insert(std::make_pair(clone->index(), clone));
  }
  std::unordered_map<int, VariablePtr>::const_iterator it_b;
  for (it_b = variable_args_.begin(); it_b != variable_args_.end(); ++it_b) {
    it_b->second->parents_.insert(std::make_pair(clone->index(), clone));
  }
  std::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = constant_args_.begin(); it_c != constant_args_.end(); ++it_c) {
    it_c->second->parents_.insert(std::make_pair(clone->index(), clone));
  }
  return clone;
}

void IGate::AddArg(int arg, const IGatePtr& gate) {
  assert(arg != 0);
  assert(std::abs(arg) == gate->index());
  assert(state_ == kNormalState);
  assert((type_ == kNotGate || type_ == kNullGate) ? args_.empty() : true);
  assert(type_ == kXorGate ? args_.size() < 2 : true);

  if (args_.count(arg)) return IGate::ProcessDuplicateArg(arg);
  if (args_.count(-arg)) return IGate::ProcessComplementArg(arg);

  args_.insert(arg);
  gate_args_.insert(std::make_pair(arg, gate));
  gate->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
}

void IGate::AddArg(int arg, const VariablePtr& variable) {
  assert(arg != 0);
  assert(std::abs(arg) == variable->index());
  assert(state_ == kNormalState);
  assert((type_ == kNotGate || type_ == kNullGate) ? args_.empty() : true);
  assert(type_ == kXorGate ? args_.size() < 2 : true);

  if (args_.count(arg)) return IGate::ProcessDuplicateArg(arg);
  if (args_.count(-arg)) return IGate::ProcessComplementArg(arg);

  args_.insert(arg);
  variable_args_.insert(std::make_pair(arg, variable));
  variable->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
}

void IGate::AddArg(int arg, const ConstantPtr& constant) {
  assert(arg != 0);
  assert(std::abs(arg) == constant->index());
  assert(state_ == kNormalState);
  assert((type_ == kNotGate || type_ == kNullGate) ? args_.empty() : true);
  assert(type_ == kXorGate ? args_.size() < 2 : true);

  if (args_.count(arg)) return IGate::ProcessDuplicateArg(arg);
  if (args_.count(-arg)) return IGate::ProcessComplementArg(arg);

  args_.insert(arg);
  constant_args_.insert(std::make_pair(arg, constant));
  constant->parents_.insert(std::make_pair(Node::index(), shared_from_this()));
}

void IGate::TransferArg(int arg, const IGatePtr& recipient) {
  assert(arg != 0);
  assert(args_.count(arg));
  args_.erase(arg);
  NodePtr node;
  if (gate_args_.count(arg)) {
    node = gate_args_.find(arg)->second;
    recipient->AddArg(arg, gate_args_.find(arg)->second);
    gate_args_.erase(arg);
  } else if (variable_args_.count(arg)) {
    node = variable_args_.find(arg)->second;
    recipient->AddArg(arg, variable_args_.find(arg)->second);
    variable_args_.erase(arg);
  } else {
    assert(constant_args_.count(arg));
    node = constant_args_.find(arg)->second;
    recipient->AddArg(arg, constant_args_.find(arg)->second);
    constant_args_.erase(arg);
  }
  assert(node->parents_.count(Node::index()));
  node->parents_.erase(Node::index());
}

void IGate::ShareArg(int arg, const IGatePtr& recipient) {
  assert(arg != 0);
  assert(args_.count(arg));
  if (gate_args_.count(arg)) {
    recipient->AddArg(arg, gate_args_.find(arg)->second);
  } else if (variable_args_.count(arg)) {
    recipient->AddArg(arg, variable_args_.find(arg)->second);
  } else {
    assert(constant_args_.count(arg));
    recipient->AddArg(arg, constant_args_.find(arg)->second);
  }
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

void IGate::JoinGate(const IGatePtr& arg_gate) {
  assert(args_.count(arg_gate->index()));  // Positive argument only.

  std::unordered_map<int, IGatePtr>::const_iterator it_g;
  for (it_g = arg_gate->gate_args_.begin();
       it_g != arg_gate->gate_args_.end(); ++it_g) {
    IGate::AddArg(it_g->first, it_g->second);
    if (state_ != kNormalState) return;
  }
  std::unordered_map<int, VariablePtr>::const_iterator it_b;
  for (it_b = arg_gate->variable_args_.begin();
       it_b != arg_gate->variable_args_.end(); ++it_b) {
    IGate::AddArg(it_b->first, it_b->second);
    if (state_ != kNormalState) return;
  }
  std::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = arg_gate->constant_args_.begin();
       it_c != arg_gate->constant_args_.end(); ++it_c) {
    IGate::AddArg(it_c->first, it_c->second);
    if (state_ != kNormalState) return;
  }

  args_.erase(arg_gate->index());  // Erase at the end to avoid the type change.
  gate_args_.erase(arg_gate->index());
  assert(arg_gate->parents_.count(Node::index()));
  arg_gate->parents_.erase(Node::index());
}

void IGate::JoinNullGate(int index) {
  assert(index != 0);
  assert(args_.count(index));
  assert(gate_args_.count(index));

  args_.erase(index);
  IGatePtr null_gate = gate_args_.find(index)->second;
  gate_args_.erase(index);
  null_gate->parents_.erase(Node::index());

  assert(null_gate->type_ == kNullGate);
  assert(null_gate->args_.size() == 1);

  int arg = *null_gate->args_.begin();
  arg *= index > 0 ? 1 : -1;  // Carry the parent's sign.

  if (!null_gate->gate_args_.empty()) {
    IGate::AddArg(arg, null_gate->gate_args_.begin()->second);
  } else if (!null_gate->constant_args_.empty()) {
    IGate::AddArg(arg, null_gate->constant_args_.begin()->second);
  } else {
    assert(!null_gate->variable_args_.empty());
    IGate::AddArg(arg, null_gate->variable_args_.begin()->second);
  }
}

void IGate::ProcessDuplicateArg(int index) {
  assert(type_ != kNotGate && type_ != kNullGate);
  assert(args_.count(index));
  switch (type_) {
    case kXorGate:
      IGate::Nullify();
      break;
    case kAtleastGate:
      // This is a very special handling of K/N duplicates.
      // @(k, [x, x, y_i]) = x & @(k-2, [y_i]) | @(k, [y_i])
      assert(vote_number_ > 1);
      assert(args_.size() > 2);
      IGatePtr clone_one = IGate::Clone();  // @(k, [y_i])

      this->EraseAllArgs();  // The main gate turns into OR with x.
      type_ = kOrGate;
      this->AddArg(clone_one->index(), clone_one);
      if (vote_number_ == 2) {  // No need for the second K/N gate.
        clone_one->TransferArg(index, shared_from_this());  // Transfered the x.
        assert(this->args_.size() == 2);
        return;
      }
      // Create the AND gate to combine with the duplicate node.
      IGatePtr and_gate(new IGate(kAndGate));
      this->AddArg(and_gate->index(), and_gate);
      clone_one->TransferArg(index, and_gate);  // Transfered the x.

      // Have to create the second K/N for vote_number > 2.
      IGatePtr clone_two = clone_one->Clone();
      clone_two->vote_number(vote_number_ - 2);  // @(k-2, [y_i])
      if (clone_two->vote_number() == 1) clone_two->type(kOrGate);
      and_gate->AddArg(clone_two->index(), clone_two);

      assert(and_gate->args().size() == 2);
      assert(this->args_.size() == 2);
  }
  if (args_.size() == 1) {
    switch (type_) {
      case kAndGate:
      case kOrGate:
        type_ = kNullGate;
        break;
      case kNandGate:
      case kNorGate:
        type_ = kNotGate;
        break;
      default:
        assert(false);  // NOT and NULL gates can't have duplicates.
    }
  }
}

void IGate::ProcessComplementArg(int index) {
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
      break;
  }
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
    {{"and", kAndGate}, {"or", kOrGate}, {"atleast", kAtleastGate},
     {"xor", kXorGate}, {"not", kNotGate}, {"nand", kNandGate},
     {"nor", kNorGate}, {"null", kNullGate}};

BooleanGraph::BooleanGraph(const GatePtr& root, bool ccf)
    : coherent_(true),
      normal_(true) {
  Node::ResetIndex();
  Variable::ResetIndex();
  std::unordered_map<std::string, NodePtr> id_to_node;
  root_ = BooleanGraph::ProcessFormula(root->formula(), ccf, &id_to_node);
}

void BooleanGraph::Print() {
  ClearNodeVisits();
  std::cerr << std::endl << this << std::endl;
}

std::shared_ptr<IGate> BooleanGraph::ProcessFormula(
    const FormulaPtr& formula,
    bool ccf,
    std::unordered_map<std::string, NodePtr>* id_to_node) {
  Operator type = kStringToType_.find(formula->type())->second;
  IGatePtr parent(new IGate(type));

  if (type != kOrGate && type != kAndGate) normal_ = false;

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
    case kNullGate:
      null_gates_.push_back(parent);
      break;
  }
  BooleanGraph::ProcessBasicEvents(parent, formula->basic_event_args(),
                                   ccf, id_to_node);

  BooleanGraph::ProcessHouseEvents(parent, formula->house_event_args(),
                                   id_to_node);

  BooleanGraph::ProcessGates(parent, formula->gate_args(), ccf, id_to_node);

  const std::set<FormulaPtr>& formulas = formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas.begin(); it_f != formulas.end(); ++it_f) {
    IGatePtr new_gate = BooleanGraph::ProcessFormula(*it_f, ccf, id_to_node);
    parent->AddArg(new_gate->index(), new_gate);
  }
  return parent;
}

void BooleanGraph::ProcessBasicEvents(
      const IGatePtr& parent,
      const std::vector<BasicEventPtr>& basic_events,
      bool ccf,
      std::unordered_map<std::string, NodePtr>* id_to_node) {
  std::vector<BasicEventPtr>::const_iterator it_b;
  for (it_b = basic_events.begin(); it_b != basic_events.end(); ++it_b) {
    BasicEventPtr basic_event = *it_b;
    if (id_to_node->count(basic_event->id())) {  // Node already exists.
      NodePtr node = id_to_node->find(basic_event->id())->second;
      if (ccf && basic_event->HasCcf()) {  // Replace with a CCF gate.
        parent->AddArg(node->index(), std::static_pointer_cast<IGate>(node));
      } else {
        parent->AddArg(node->index(), std::static_pointer_cast<Variable>(node));
      }
    } else {  // Create a new node.
      if (ccf && basic_event->HasCcf()) {  // Create a CCF gate.
        GatePtr ccf_gate = basic_event->ccf_gate();
        IGatePtr new_gate =
            BooleanGraph::ProcessFormula(ccf_gate->formula(), ccf, id_to_node);
        parent->AddArg(new_gate->index(), new_gate);
        id_to_node->insert(std::make_pair(basic_event->id(), new_gate));
      } else {
        basic_events_.push_back(basic_event);
        VariablePtr new_basic(new Variable());  // Sequential indexation.
        assert(basic_events_.size() == new_basic->index());
        parent->AddArg(new_basic->index(), new_basic);
        id_to_node->insert(std::make_pair(basic_event->id(), new_basic));
      }
    }
  }
}

void BooleanGraph::ProcessHouseEvents(
      const IGatePtr& parent,
      const std::vector<HouseEventPtr>& house_events,
      std::unordered_map<std::string, NodePtr>* id_to_node) {
  std::vector<HouseEventPtr>::const_iterator it_h;
  for (it_h = house_events.begin(); it_h != house_events.end(); ++it_h) {
    HouseEventPtr house = *it_h;
    if (id_to_node->count(house->id())) {
      NodePtr node = id_to_node->find(house->id())->second;
      parent->AddArg(node->index(), std::static_pointer_cast<Constant>(node));
    } else {
      ConstantPtr constant(new Constant(house->state()));
      parent->AddArg(constant->index(), constant);
      id_to_node->insert(std::make_pair(house->id(), constant));
      constants_.push_back(constant);
    }
  }
}

void BooleanGraph::ProcessGates(
      const IGatePtr& parent,
      const std::vector<GatePtr>& gates,
      bool ccf,
      std::unordered_map<std::string, NodePtr>* id_to_node) {
  std::vector<GatePtr>::const_iterator it_g;
  for (it_g = gates.begin(); it_g != gates.end(); ++it_g) {
    GatePtr gate = *it_g;
    if (id_to_node->count(gate->id())) {
      NodePtr node = id_to_node->find(gate->id())->second;
      parent->AddArg(node->index(), std::static_pointer_cast<IGate>(node));
    } else {
      IGatePtr new_gate = BooleanGraph::ProcessFormula(gate->formula(), ccf,
                                                       id_to_node);
      parent->AddArg(new_gate->index(), new_gate);
      id_to_node->insert(std::make_pair(gate->id(), new_gate));
    }
  }
}

void BooleanGraph::ClearGateMarks() {
  BooleanGraph::ClearGateMarks(root_);
}

void BooleanGraph::ClearGateMarks(const IGatePtr& gate) {
  if (!gate->mark()) return;
  gate->mark(false);
  std::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_args().begin(); it != gate->gate_args().end(); ++it) {
    BooleanGraph::ClearGateMarks(it->second);
  }
}

void BooleanGraph::ClearNodeVisits() {
  LOG(DEBUG5) << "Clearing node visit times...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearNodeVisits(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node visit times are clear!";
}

void BooleanGraph::ClearNodeVisits(const IGatePtr& gate) {
  if (gate->mark()) return;
  gate->mark(true);

  if (gate->Visited()) gate->ClearVisits();

  std::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_args().begin(); it != gate->gate_args().end(); ++it) {
    BooleanGraph::ClearNodeVisits(it->second);
  }
  std::unordered_map<int, VariablePtr>::const_iterator it_b;
  for (it_b = gate->variable_args().begin();
       it_b != gate->variable_args().end(); ++it_b) {
    if (it_b->second->Visited()) it_b->second->ClearVisits();
  }
  std::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = gate->constant_args().begin();
       it_c != gate->constant_args().end(); ++it_c) {
    if (it_c->second->Visited()) it_c->second->ClearVisits();
  }
}

void BooleanGraph::ClearOptiValues() {
  LOG(DEBUG5) << "Clearing OptiValues...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearOptiValues(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node OptiValues are clear!";
}

void BooleanGraph::ClearOptiValues(const IGatePtr& gate) {
  if (gate->mark()) return;
  gate->mark(true);

  gate->opti_value(0);
  gate->ResetArgFailure();
  std::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_args().begin(); it != gate->gate_args().end(); ++it) {
    BooleanGraph::ClearOptiValues(it->second);
  }
  std::unordered_map<int, VariablePtr>::const_iterator it_b;
  for (it_b = gate->variable_args().begin();
       it_b != gate->variable_args().end(); ++it_b) {
    it_b->second->opti_value(0);
  }
  assert(gate->constant_args().empty());
}

std::ostream& operator<<(std::ostream& os,
                         const std::shared_ptr<Constant>& constant) {
  if (constant->Visited()) return os;
  constant->Visit(1);
  std::string state = constant->state() ? "true" : "false";
  os << "s(H" << constant->index() << ") = " << state << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const std::shared_ptr<Variable>& variable) {
  if (variable->Visited()) return os;
  variable->Visit(1);
  os << "p(B" << variable->index() << ") = " << 1 << std::endl;
  return os;
}

namespace {

/// @struct FormulaSig
/// Gate formula signature for printing in the shorthand format.
struct FormulaSig {
  std::string begin;  ///< Beginning of the formula string.
  std::string op;  ///< Operator between the formula arguments.
  std::string end;  ///< The end of the formula string.
};

/// Provides proper formatting clues for gate formulas.
///
/// @param[in] gate The gate with the formula to be printed.
///
/// @returns The beginning, operator, and end strings for the formula.
const FormulaSig GetFormulaSig(const std::shared_ptr<const IGate>& gate) {
  FormulaSig sig = {"(", "", ")"};  // Defaults for most gate types.

  switch (gate->type()) {
    case kNandGate:
      sig.begin = "~(";  // Fall-through to AND gate.
    case kAndGate:
      sig.op = " & ";
      break;
    case kNorGate:
      sig.begin = "~(";  // Fall-through to OR gate.
    case kOrGate:
      sig.op = " | ";
      break;
    case kXorGate:
      sig.op = " ^ ";
      break;
    case kNotGate:
      sig.begin = "~(";  // Parentheses are for cases of NOT(NOT Arg).
      break;
    case kNullGate:
      sig.begin = "";  // No need for the parentheses.
      sig.end = "";
      break;
    case kAtleastGate:
      sig.begin = "@(" + std::to_string(gate->vote_number()) + ", [";
      sig.op = ", ";
      sig.end = "])";
      break;
  }
  return sig;
}

/// Provides special formatting for indexed gate names.
///
/// @param[in] gate The gate which name must be created.
///
/// @returns The name of the gate with extra information about its state.
const std::string GetName(const std::shared_ptr<const IGate>& gate) {
  std::string name = "G";
  if (gate->state() == kNormalState) {
    if (gate->IsModule()) name += "M";
  } else {  // This gate has become constant.
    name += "C";
  }
  name += std::to_string(gate->index());
  return name;
}

}  // namespace

std::ostream& operator<<(std::ostream& os,
                         const std::shared_ptr<IGate>& gate) {
  if (gate->Visited()) return os;
  gate->Visit(1);
  std::string name = GetName(gate);
  if (gate->state() != kNormalState) {
    std::string state = gate->state() == kNullState ? "false" : "true";
    os << "s(" << name << ") = " << state << std::endl;
    return os;
  }
  std::string formula = "";  // The formula of the gate for printing.
  const FormulaSig sig = GetFormulaSig(gate);  // Formatting for the formula.
  int num_args = gate->args().size();  // The number of arguments to print.

  typedef std::shared_ptr<IGate> IGatePtr;
  std::unordered_map<int, IGatePtr>::const_iterator it_gate;
  for (it_gate = gate->gate_args().begin(); it_gate != gate->gate_args().end();
       ++it_gate) {
    if (it_gate->first < 0) formula += "~";  // Negation.
    formula += GetName(it_gate->second);

    if (--num_args) formula += sig.op;

    os << it_gate->second;
  }

  typedef std::shared_ptr<Variable> VariablePtr;
  std::unordered_map<int, VariablePtr>::const_iterator it_basic;
  for (it_basic = gate->variable_args().begin();
       it_basic != gate->variable_args().end(); ++it_basic) {
    if (it_basic->first < 0) formula += "~";  // Negation.
    int index = it_basic->second->index();
    formula += "B" + std::to_string(index);

    if (--num_args) formula += sig.op;

    os << it_basic->second;
  }

  typedef std::shared_ptr<Constant> ConstantPtr;
  std::unordered_map<int, ConstantPtr>::const_iterator it_const;
  for (it_const = gate->constant_args().begin();
       it_const != gate->constant_args().end(); ++it_const) {
    if (it_const->first < 0) formula += "~";  // Negation.
    int index = it_const->second->index();
    formula += "H" + std::to_string(index);

    if (--num_args) formula += sig.op;

    os << it_const->second;
  }
  os << name << " := " << sig.begin << formula << sig.end << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const BooleanGraph* ft) {
  os << "BooleanGraph" << std::endl << std::endl;
  os << ft->root();
  return os;
}

}  // namespace scram
