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
/// to make manipulations and transformations of the graph
/// easier to achieve for graph algorithms.

#include "boolean_graph.h"

#include <utility>

#include "event.h"
#include "logger.h"

namespace scram {

int Node::next_index_ = 1e6;  // 1 million basic events per fault tree is crazy!

Node::Node() noexcept : Node::Node(next_index_++) {}

Node::Node(int index) noexcept
    : index_(index),
      opti_value_(0),
      pos_count_(0),
      neg_count_(0) {
  std::fill(visits_, visits_ + 3, 0);
}

Node::~Node() {}  // Empty body for pure virtual destructor.

Constant::Constant(bool state) noexcept : Node(), state_(state) {}

int Variable::next_variable_ = 1;

Variable::Variable() noexcept : Node(next_variable_++) {}

IGate::IGate(const Operator& type) noexcept
    : Node(),
      type_(type),
      state_(kNormalState),
      vote_number_(-1),
      mark_(false),
      min_time_(0),
      max_time_(0),
      module_(false),
      num_failed_args_(0) {}

std::shared_ptr<IGate> IGate::Clone() noexcept {
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

void IGate::AddArg(int arg, const IGatePtr& gate) noexcept {
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

void IGate::AddArg(int arg, const VariablePtr& variable) noexcept {
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

void IGate::AddArg(int arg, const ConstantPtr& constant) noexcept {
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

void IGate::TransferArg(int arg, const IGatePtr& recipient) noexcept {
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

void IGate::ShareArg(int arg, const IGatePtr& recipient) noexcept {
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

void IGate::InvertArgs() noexcept {
  std::set<int> args(args_);  // Not to mess the iterator.
  std::set<int>::iterator it;
  for (it = args.begin(); it != args.end(); ++it) {
    IGate::InvertArg(*it);
  }
}

void IGate::InvertArg(int existing_arg) noexcept {
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

void IGate::JoinGate(const IGatePtr& arg_gate) noexcept {
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

void IGate::JoinNullGate(int index) noexcept {
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

void IGate::ProcessDuplicateArg(int index) noexcept {
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
      if (args_.size() == 2) {
        assert(vote_number_ == 2);
        this->EraseArg(index);
        this->type_ = kNullGate;
        return;
      }
      assert(args_.size() > 2);
      IGatePtr clone_one = IGate::Clone();  // @(k, [y_i])

      this->EraseAllArgs();  // The main gate turns into OR with x.
      type_ = kOrGate;
      this->AddArg(clone_one->index(), clone_one);
      if (vote_number_ == 2) {  // No need for the second K/N gate.
        clone_one->TransferArg(index, shared_from_this());  // Transfered the x.
        assert(this->args_.size() == 2);
      } else {
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
      if (clone_one->args().size() == clone_one->vote_number())
        clone_one->type(kAndGate);
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

void IGate::ProcessComplementArg(int index) noexcept {
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

void IGate::ArgFailed() noexcept {
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

BooleanGraph::BooleanGraph(const GatePtr& root, bool ccf) noexcept
    : coherent_(true),
      normal_(true) {
  Node::ResetIndex();
  Variable::ResetIndex();
  ProcessedNodes nodes;
  root_ = BooleanGraph::ProcessFormula(root->formula(), ccf, &nodes);
}

void BooleanGraph::Print() {
  ClearNodeVisits();
  std::cerr << std::endl << this << std::endl;
}

std::shared_ptr<IGate> BooleanGraph::ProcessFormula(
    const FormulaPtr& formula,
    bool ccf,
    ProcessedNodes* nodes) noexcept {
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
  BooleanGraph::ProcessBasicEvents(parent, formula->basic_event_args(), ccf,
                                   nodes);

  BooleanGraph::ProcessHouseEvents(parent, formula->house_event_args(), nodes);

  BooleanGraph::ProcessGates(parent, formula->gate_args(), ccf, nodes);

  for (const FormulaPtr& sub_form : formula->formula_args()) {
    IGatePtr new_gate = BooleanGraph::ProcessFormula(sub_form, ccf, nodes);
    parent->AddArg(new_gate->index(), new_gate);
  }
  return parent;
}

void BooleanGraph::ProcessBasicEvents(
    const IGatePtr& parent,
    const std::vector<BasicEventPtr>& basic_events,
    bool ccf,
    ProcessedNodes* nodes) noexcept {
  for (const auto& basic_event : basic_events) {
    if (ccf && basic_event->HasCcf()) {  // Replace with a CCF gate.
      if (nodes->gates.count(basic_event->id())) {
        IGatePtr ccf_gate = nodes->gates.find(basic_event->id())->second;
        parent->AddArg(ccf_gate->index(), ccf_gate);
      } else {
        GatePtr ccf_gate = basic_event->ccf_gate();
        IGatePtr new_gate =
            BooleanGraph::ProcessFormula(ccf_gate->formula(), ccf, nodes);
        parent->AddArg(new_gate->index(), new_gate);
        nodes->gates.emplace(basic_event->id(), new_gate);
      }
    } else {
      if (nodes->variables.count(basic_event->id())) {
        VariablePtr var = nodes->variables.find(basic_event->id())->second;
        parent->AddArg(var->index(), var);
      } else {
        basic_events_.push_back(basic_event);
        VariablePtr new_basic(new Variable());  // Sequential indexation.
        assert(basic_events_.size() == new_basic->index());
        parent->AddArg(new_basic->index(), new_basic);
        nodes->variables.emplace(basic_event->id(), new_basic);
      }
    }
  }
}

void BooleanGraph::ProcessHouseEvents(
    const IGatePtr& parent,
    const std::vector<HouseEventPtr>& house_events,
    ProcessedNodes* nodes) noexcept {
  for (const auto& house : house_events) {
    if (nodes->constants.count(house->id())) {
      ConstantPtr constant = nodes->constants.find(house->id())->second;
      parent->AddArg(constant->index(), constant);
    } else {
      ConstantPtr constant(new Constant(house->state()));
      parent->AddArg(constant->index(), constant);
      nodes->constants.emplace(house->id(), constant);
      constants_.push_back(constant);
    }
  }
}

void BooleanGraph::ProcessGates(const IGatePtr& parent,
                                const std::vector<GatePtr>& gates,
                                bool ccf,
                                ProcessedNodes* nodes) noexcept {
  for (const auto& gate : gates) {
    if (nodes->gates.count(gate->id())) {
      IGatePtr node = nodes->gates.find(gate->id())->second;
      parent->AddArg(node->index(), node);
    } else {
      IGatePtr new_gate = BooleanGraph::ProcessFormula(gate->formula(), ccf,
                                                       nodes);
      parent->AddArg(new_gate->index(), new_gate);
      nodes->gates.emplace(gate->id(), new_gate);
    }
  }
}

void BooleanGraph::ClearGateMarks() noexcept {
  BooleanGraph::ClearGateMarks(root_);
}

void BooleanGraph::ClearGateMarks(const IGatePtr& gate) noexcept {
  if (!gate->mark()) return;
  gate->mark(false);
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::ClearGateMarks(arg.second);
  }
}

void BooleanGraph::ClearNodeVisits() noexcept {
  LOG(DEBUG5) << "Clearing node visit times...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearNodeVisits(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node visit times are clear!";
}

void BooleanGraph::ClearNodeVisits(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  if (gate->Visited()) gate->ClearVisits();

  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::ClearNodeVisits(arg.second);
  }
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    if (arg.second->Visited()) arg.second->ClearVisits();
  }
  for (const std::pair<int, ConstantPtr>& arg : gate->constant_args()) {
    if (arg.second->Visited()) arg.second->ClearVisits();
  }
}

void BooleanGraph::ClearOptiValues() noexcept {
  LOG(DEBUG5) << "Clearing OptiValues...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearOptiValues(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node OptiValues are clear!";
}

void BooleanGraph::ClearOptiValues(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  gate->opti_value(0);
  gate->ResetArgFailure();
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::ClearOptiValues(arg.second);
  }
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    arg.second->opti_value(0);
  }
  assert(gate->constant_args().empty());
}

void BooleanGraph::ClearOptiValuesFast(const IGatePtr& gate) noexcept {
  if (!gate->opti_value()) return;
  gate->opti_value(0);
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::ClearOptiValuesFast(arg.second);
  }
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    if (arg.second->opti_value()) {
      arg.second->opti_value(0);
      break;  // Only one variable is dirty.
    }
  }
  assert(gate->constant_args().empty());
}

void BooleanGraph::ClearNodeCounts() noexcept {
  LOG(DEBUG5) << "Clearing node counts...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearNodeCounts(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node counts are clear!";
}

void BooleanGraph::ClearNodeCounts(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  gate->ResetCount();
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::ClearNodeCounts(arg.second);
  }
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    arg.second->ResetCount();
  }
  assert(gate->constant_args().empty());
}

void BooleanGraph::TestGateMarks(const IGatePtr& gate) noexcept {
  assert(!gate->mark());
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::TestGateMarks(arg.second);
  }
}

void BooleanGraph::TestOptiValues(const IGatePtr& gate) noexcept {
  assert(!gate->opti_value());
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    BooleanGraph::TestOptiValues(arg.second);
  }
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    assert(!arg.second->opti_value());
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

std::ostream& operator<<(std::ostream& os, const std::shared_ptr<IGate>& gate) {
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
  for (const std::pair<int, IGatePtr>& node : gate->gate_args()) {
    if (node.first < 0) formula += "~";  // Negation.
    formula += GetName(node.second);

    if (--num_args) formula += sig.op;

    os << node.second;
  }

  typedef std::shared_ptr<Variable> VariablePtr;
  for (const std::pair<int, VariablePtr>& basic : gate->variable_args()) {
    if (basic.first < 0) formula += "~";  // Negation.
    int index = basic.second->index();
    formula += "B" + std::to_string(index);

    if (--num_args) formula += sig.op;

    os << basic.second;
  }

  typedef std::shared_ptr<Constant> ConstantPtr;
  for (const std::pair<int, ConstantPtr>& constant : gate->constant_args()) {
    if (constant.first < 0) formula += "~";  // Negation.
    int index = constant.second->index();
    formula += "H" + std::to_string(index);

    if (--num_args) formula += sig.op;

    os << constant.second;
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
