/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#include "logger.h"

namespace scram {

int Node::next_index_ = 1e6;  // Limit for basic events per fault tree!

void NodeParentManager::AddParent(const IGatePtr& gate) {
  parents_.emplace(gate->index(), gate);
}

Node::Node() noexcept : Node(next_index_++) {}

Node::Node(int index) noexcept
    : index_(index),
      order_(0),
      visits_{},
      opti_value_(0),
      pos_count_(0),
      neg_count_(0) {}

Node::~Node() = default;

Constant::Constant(bool state) noexcept : state_(state) {}

int Variable::next_variable_ = 1;

Variable::Variable() noexcept : Node(next_variable_++) {}

IGate::IGate(Operator type) noexcept
    : type_(type),
      state_(kNormalState),
      vote_number_(0),
      mark_(false),
      descendant_(0),
      min_time_(0),
      max_time_(0),
      module_(false),
      coherent_(false) {}

IGatePtr IGate::Clone() noexcept {
  BLOG(DEBUG5, module_) << "WARNING: Cloning module G" << Node::index();
  auto clone = std::make_shared<IGate>(type_);  // The same type.
  clone->coherent_ = coherent_;
  clone->vote_number_ = vote_number_;  // Copy vote number in case it is K/N.
  // Getting arguments copied.
  clone->args_ = args_;
  clone->gate_args_ = gate_args_;
  clone->variable_args_ = variable_args_;
  clone->constant_args_ = constant_args_;
  // Introducing the new parent to the args.
  for (const auto& arg : gate_args_) arg.second->AddParent(clone);
  for (const auto& arg : variable_args_) arg.second->AddParent(clone);
  for (const auto& arg : constant_args_) arg.second->AddParent(clone);
  return clone;
}

void IGate::TransferArg(int index, const IGatePtr& recipient) noexcept {
  assert(index != 0);
  assert(args_.count(index));
  args_.erase(index);
  NodePtr node;
  if (gate_args_.count(index)) {
    node = gate_args_.find(index)->second;
    recipient->AddArg(index, gate_args_.find(index)->second);
    gate_args_.erase(index);
  } else if (variable_args_.count(index)) {
    node = variable_args_.find(index)->second;
    recipient->AddArg(index, variable_args_.find(index)->second);
    variable_args_.erase(index);
  } else {
    assert(constant_args_.count(index));
    node = constant_args_.find(index)->second;
    recipient->AddArg(index, constant_args_.find(index)->second);
    constant_args_.erase(index);
  }
  node->EraseParent(Node::index());
}

void IGate::ShareArg(int index, const IGatePtr& recipient) noexcept {
  assert(index != 0);
  assert(args_.count(index));
  if (gate_args_.count(index)) {
    recipient->AddArg(index, gate_args_.find(index)->second);
  } else if (variable_args_.count(index)) {
    recipient->AddArg(index, variable_args_.find(index)->second);
  } else {
    assert(constant_args_.count(index));
    recipient->AddArg(index, constant_args_.find(index)->second);
  }
}

void IGate::InvertArgs() noexcept {
  std::set<int> inverted_args;
  for (int index : args_) inverted_args.insert(inverted_args.begin(), -index);
  args_ = std::move(inverted_args);

  std::unordered_map<int, IGatePtr> inverted_gates;
  for (const auto& arg : gate_args_)
    inverted_gates.emplace(-arg.first, arg.second);
  gate_args_ = std::move(inverted_gates);

  std::unordered_map<int, VariablePtr> inverted_vars;
  for (const auto& arg : variable_args_)
    inverted_vars.emplace(-arg.first, arg.second);
  variable_args_ = std::move(inverted_vars);

  std::unordered_map<int, ConstantPtr> inverted_consts;
  for (const auto& arg : constant_args_)
    inverted_consts.emplace(-arg.first, arg.second);
  constant_args_ = std::move(inverted_consts);
}

void IGate::InvertArg(int existing_arg) noexcept {
  assert(args_.count(existing_arg));
  assert(!args_.count(-existing_arg));
  args_.erase(existing_arg);
  args_.insert(-existing_arg);
  if (gate_args_.count(existing_arg)) {
    IGatePtr arg = gate_args_.find(existing_arg)->second;
    gate_args_.erase(existing_arg);
    gate_args_.emplace(-existing_arg, arg);
  } else if (variable_args_.count(existing_arg)) {
    VariablePtr arg = variable_args_.find(existing_arg)->second;
    variable_args_.erase(existing_arg);
    variable_args_.emplace(-existing_arg, arg);
  } else {
    ConstantPtr arg = constant_args_.find(existing_arg)->second;
    constant_args_.erase(existing_arg);
    constant_args_.emplace(-existing_arg, arg);
  }
}

void IGate::CoalesceGate(const IGatePtr& arg_gate) noexcept {
  assert(args_.count(arg_gate->index()) && "Cannot join complement gate.");
  assert(arg_gate->state() == kNormalState && "Impossible to join.");
  assert(!arg_gate->args().empty() && "Corrupted gate.");

  for (const auto& arg : arg_gate->gate_args_) {
    IGate::AddArg(arg.first, arg.second);
    if (state_ != kNormalState) return;
  }
  for (const auto& arg : arg_gate->variable_args_) {
    IGate::AddArg(arg.first, arg.second);
    if (state_ != kNormalState) return;
  }
  for (const auto& arg : arg_gate->constant_args_) {
    IGate::AddArg(arg.first, arg.second);
    if (state_ != kNormalState) return;
  }

  args_.erase(arg_gate->index());  // Erase at the end to avoid the type change.
  gate_args_.erase(arg_gate->index());
  arg_gate->EraseParent(Node::index());
}

void IGate::JoinNullGate(int index) noexcept {
  assert(index != 0);
  assert(args_.count(index));
  assert(gate_args_.count(index));

  args_.erase(index);
  IGatePtr null_gate = gate_args_.find(index)->second;
  gate_args_.erase(index);
  null_gate->EraseParent(Node::index());

  assert(null_gate->type_ == kNull);
  assert(null_gate->args_.size() == 1);

  int arg_index = *null_gate->args_.begin();
  arg_index *= index > 0 ? 1 : -1;  // Carry the parent's sign.

  if (!null_gate->gate_args_.empty()) {
    IGate::AddArg(arg_index, null_gate->gate_args_.begin()->second);
  } else if (!null_gate->variable_args_.empty()) {
    IGate::AddArg(arg_index, null_gate->variable_args_.begin()->second);
  } else {
    assert(!null_gate->constant_args_.empty());
    IGate::AddArg(arg_index, null_gate->constant_args_.begin()->second);
  }
}

void IGate::ProcessConstantArg(const NodePtr& arg, bool state) noexcept {
  int index = IGate::GetArgSign(arg) * arg->index();
  if (index < 0) state = !state;
  if (state) {  // Unity state or True index.
    IGate::ProcessTrueArg(index);
  } else {  // Null state or False index.
    IGate::ProcessFalseArg(index);
  }
}

void IGate::ProcessTrueArg(int index) noexcept {
  switch (type_) {
    case kNull:
    case kOr:
      IGate::MakeUnity();
      break;
    case kNand:
    case kAnd:
      IGate::RemoveConstantArg(index);
      break;
    case kNor:
    case kNot:
      IGate::Nullify();
      break;
    case kXor:  // Special handling due to its internal negation.
      assert(args_.size() == 2);
      IGate::EraseArg(index);
      assert(args_.size() == 1);
      type_ = kNot;
      break;
    case kVote:  // (K - 1) / (N - 1).
      assert(args_.size() > 2);
      IGate::EraseArg(index);
      assert(vote_number_ > 0);
      --vote_number_;
      if (vote_number_ == 1) type_ = kOr;
      break;
  }
}

void IGate::ProcessFalseArg(int index) noexcept {
  switch (type_) {
    case kNor:
    case kXor:
    case kOr:
      IGate::RemoveConstantArg(index);
      break;
    case kNull:
    case kAnd:
      IGate::Nullify();
      break;
    case kNand:
    case kNot:
      IGate::MakeUnity();
      break;
    case kVote:  // K / (N - 1).
      assert(args_.size() > 2);
      IGate::EraseArg(index);
      if (vote_number_ == args_.size()) type_ = kAnd;
      break;
  }
}

void IGate::RemoveConstantArg(int index) noexcept {
  assert(args_.size() > 1 && "One-arg gate must have become constant.");
  IGate::EraseArg(index);
  if (args_.size() == 1) {
    switch (type_) {
      case kXor:
      case kOr:
      case kAnd:
        type_ = kNull;
        break;
      case kNor:
      case kNand:
        type_ = kNot;
        break;
      default:
        assert(false && "NULL/NOT one-arg gates should not appear.");
    }
  }  // More complex cases with K/N gates are handled by the caller functions.
}

void IGate::EraseArg(int index) noexcept {
  assert(index != 0);
  assert(args_.count(index));
  args_.erase(index);
  NodePtr node;
  if (gate_args_.count(index)) {
    node = gate_args_.find(index)->second;
    gate_args_.erase(index);
  } else if (variable_args_.count(index)) {
    node = variable_args_.find(index)->second;
    variable_args_.erase(index);
  } else {
    assert(constant_args_.count(index));
    node = constant_args_.find(index)->second;
    constant_args_.erase(index);
  }
  node->EraseParent(Node::index());
}

void IGate::EraseAllArgs() noexcept {
  args_.clear();
  for (const auto& arg : gate_args_) arg.second->EraseParent(Node::index());
  gate_args_.clear();
  for (const auto& arg : variable_args_) arg.second->EraseParent(Node::index());
  variable_args_.clear();
  for (const auto& arg : constant_args_) arg.second->EraseParent(Node::index());
  constant_args_.clear();
}

void IGate::ProcessDuplicateArg(int index) noexcept {
  assert(type_ != kNot && type_ != kNull);
  assert(args_.count(index));
  LOG(DEBUG5) << "Handling duplicate argument for G" << Node::index();
  if (type_ == kVote)
    return IGate::ProcessVoteGateDuplicateArg(index);

  if (args_.size() == 1) {
    LOG(DEBUG5) << "Handling the case of one-arg duplicate argument!";
    switch (type_) {
      case kAnd:
      case kOr:
        type_ = kNull;
        break;
      case kNand:
      case kNor:
        type_ = kNot;
        break;
      case kXor:
        LOG(DEBUG5) << "Handling special case of XOR duplicate argument!";
        IGate::Nullify();
        break;
      default:
        assert(false && "NOT and NULL gates can't have duplicates.");
    }
  }
}

void IGate::ProcessVoteGateDuplicateArg(int index) noexcept {
  LOG(DEBUG5) << "Handling special case of K/N duplicate argument!";
  assert(type_ == kVote);
  // This is a very special handling of K/N duplicates.
  // @(k, [x, x, y_i]) = x & @(k-2, [y_i]) | @(k, [y_i])
  assert(vote_number_ > 1);
  assert(args_.size() >= vote_number_);
  if (args_.size() == 2) {  // @(2, [x, x, z]) = x
    assert(vote_number_ == 2);
    this->EraseArg(index);
    this->type_ = kNull;
    return;
  }
  if (vote_number_ == args_.size()) {  // @(k, [y_i]) is NULL set.
    assert(vote_number_ > 2 && "Corrupted number of gate arguments.");
    IGatePtr clone_two = this->Clone();
    clone_two->vote_number(vote_number_ - 2);  // @(k-2, [y_i])
    this->EraseAllArgs();
    this->type_ = kAnd;
    clone_two->TransferArg(index, shared_from_this());  // Transfered the x.
    if (clone_two->vote_number() == 1) clone_two->type(kOr);
    this->AddArg(clone_two->index(), clone_two);
    return;
  }
  assert(args_.size() > 2);
  IGatePtr clone_one = this->Clone();  // @(k, [y_i])

  this->EraseAllArgs();  // The main gate turns into OR with x.
  type_ = kOr;
  this->AddArg(clone_one->index(), clone_one);
  if (vote_number_ == 2) {  // No need for the second K/N gate.
    clone_one->TransferArg(index, shared_from_this());  // Transfered the x.
    assert(this->args_.size() == 2);
  } else {
    // Create the AND gate to combine with the duplicate node.
    auto and_gate = std::make_shared<IGate>(kAnd);
    this->AddArg(and_gate->index(), and_gate);
    clone_one->TransferArg(index, and_gate);  // Transfered the x.

    // Have to create the second K/N for vote_number > 2.
    IGatePtr clone_two = clone_one->Clone();
    clone_two->vote_number(vote_number_ - 2);  // @(k-2, [y_i])
    if (clone_two->vote_number() == 1) clone_two->type(kOr);
    and_gate->AddArg(clone_two->index(), clone_two);

    assert(and_gate->args().size() == 2);
    assert(this->args_.size() == 2);
  }
  assert(clone_one->vote_number() <= clone_one->args().size());
  if (clone_one->args().size() == clone_one->vote_number())
    clone_one->type(kAnd);
}

void IGate::ProcessComplementArg(int index) noexcept {
  assert(type_ != kNot && type_ != kNull);
  assert(args_.count(-index));
  LOG(DEBUG5) << "Handling complement argument for G" << Node::index();
  switch (type_) {
    case kNor:
    case kAnd:
      IGate::Nullify();
      break;
    case kNand:
    case kXor:
    case kOr:
      IGate::MakeUnity();
      break;
    case kVote:
      LOG(DEBUG5) << "Handling special case of K/N complement argument!";
      assert(vote_number_ > 1 && "Vote number is wrong.");
      assert((args_.size() + 1) > vote_number_ && "Malformed K/N gate.");
      // @(k, [x, x', y_i]) = @(k-1, [y_i])
      IGate::EraseArg(-index);
      --vote_number_;
      if (args_.size() == 1) {
        type_ = kNull;
      } else if (vote_number_ == 1) {
        type_ = kOr;
      } else if (vote_number_ == args_.size()) {
        type_ = kAnd;
      }
      break;
    default:
      assert(false && "Unexpected gate type for complement arg processing.");
  }
}

const std::map<std::string, Operator> BooleanGraph::kStringToType_ = {
    {"and", kAnd},
    {"or", kOr},
    {"atleast", kVote},
    {"xor", kXor},
    {"not", kNot},
    {"nand", kNand},
    {"nor", kNor},
    {"null", kNull}};

BooleanGraph::BooleanGraph(const GatePtr& root, bool ccf) noexcept
    : root_sign_(1),
      coherent_(true),
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

IGatePtr BooleanGraph::ProcessFormula(const FormulaPtr& formula, bool ccf,
                                      ProcessedNodes* nodes) noexcept {
  Operator type = kStringToType_.find(formula->type())->second;
  auto parent = std::make_shared<IGate>(type);

  if (type != kOr && type != kAnd) normal_ = false;

  switch (type) {
    case kNot:
    case kNand:
    case kNor:
    case kXor:
      coherent_ = false;
      break;
    case kVote:
      parent->vote_number(formula->vote_number());
      break;
    case kNull:
      null_gates_.push_back(parent);
      break;
    default:
      assert((type == kOr || type == kAnd) && "Unexpected gate type.");
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
        auto new_basic = std::make_shared<Variable>();  // Sequential indices.
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
      auto constant = std::make_shared<Constant>(house->state());
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
  for (const auto& arg : gate->gate_args()) {
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

  for (const auto& arg : gate->gate_args()) {
    BooleanGraph::ClearNodeVisits(arg.second);
  }
  for (const auto& arg : gate->variable_args()) {
    if (arg.second->Visited()) arg.second->ClearVisits();
  }
  for (const auto& arg : gate->constant_args()) {
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
  for (const auto& arg : gate->gate_args()) {
    BooleanGraph::ClearOptiValues(arg.second);
  }
  for (const auto& arg : gate->variable_args()) {
    arg.second->opti_value(0);
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
  for (const auto& arg : gate->gate_args()) {
    BooleanGraph::ClearNodeCounts(arg.second);
  }
  for (const auto& arg : gate->variable_args()) {
    arg.second->ResetCount();
  }
  assert(gate->constant_args().empty());
}

void BooleanGraph::ClearDescendantMarks() noexcept {
  LOG(DEBUG5) << "Clearing gate descendant marks...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearDescendantMarks(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Descendant marks are clear!";
}

void BooleanGraph::ClearDescendantMarks(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  gate->descendant(0);
  for (const auto& arg : gate->gate_args()) {
    BooleanGraph::ClearDescendantMarks(arg.second);
  }
}

void BooleanGraph::ClearNodeOrders() noexcept {
  LOG(DEBUG5) << "Clearing node order marks...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearNodeOrders(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node order marks are clear!";
}

void BooleanGraph::ClearNodeOrders(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  if (gate->order()) gate->order(0);
  for (const auto& arg : gate->gate_args()) {
    BooleanGraph::ClearNodeOrders(arg.second);
  }
  for (const auto& arg : gate->variable_args()) {
    if (arg.second->order()) arg.second->order(0);
  }
  assert(gate->constant_args().empty());
}

void BooleanGraph::GraphLogger::RegisterRoot(const IGatePtr& gate) noexcept {
  gates.insert(gate->index());
}

void BooleanGraph::GraphLogger::Log(const IGatePtr& gate) noexcept {
  ++gate_types[gate->type()];
  if (gate->IsModule()) ++num_modules;
  for (const auto& arg : gate->gate_args()) gates.insert(arg.first);
  for (const auto& arg : gate->variable_args()) variables.insert(arg.first);
  for (const auto& arg : gate->constant_args()) constants.insert(arg.first);
}

void BooleanGraph::Log() noexcept {
  if (DEBUG4 > scram::Logger::report_level()) return;
  BooleanGraph::ClearGateMarks();
  GraphLogger* logger = new GraphLogger();
  logger->RegisterRoot(root_);
  BooleanGraph::GatherInformation(root_, logger);
  LOG(DEBUG4) << "Boolean graph with root G" << root_->index();
  LOG(DEBUG4) << "Total # of gates: " << logger->Count(logger->gates);
  LOG(DEBUG4) << "# of modules: " << logger->num_modules;
  LOG(DEBUG4) << "# of gates with negative indices: "
              << logger->CountComplements(logger->gates);
  LOG(DEBUG4) << "# of gates with positive and negative indices: "
              << logger->CountOverlap(logger->gates);

  BLOG(DEBUG5, logger->gate_types[kAnd]) << "AND gates: "
                                         << logger->gate_types[kAnd];
  BLOG(DEBUG5, logger->gate_types[kOr]) << "OR gates: "
                                        << logger->gate_types[kOr];
  BLOG(DEBUG5, logger->gate_types[kVote])
      << "K/N gates: " << logger->gate_types[kVote];
  BLOG(DEBUG5, logger->gate_types[kXor]) << "XOR gates: "
                                         << logger->gate_types[kXor];
  BLOG(DEBUG5, logger->gate_types[kNot]) << "NOT gates: "
                                         << logger->gate_types[kNot];
  BLOG(DEBUG5, logger->gate_types[kNand])
      << "NAND gates: " << logger->gate_types[kNand];
  BLOG(DEBUG5, logger->gate_types[kNor]) << "NOR gates: "
                                         << logger->gate_types[kNor];
  BLOG(DEBUG5, logger->gate_types[kNull])
      << "NULL gates: " << logger->gate_types[kNull];

  LOG(DEBUG4) << "Total # of variables: " << logger->Count(logger->variables);
  LOG(DEBUG4) << "# of variables with negative indices: "
              << logger->CountComplements(logger->variables);
  LOG(DEBUG4) << "# of variables with positive and negative indices: "
              << logger->CountOverlap(logger->variables);

  BLOG(DEBUG4, !logger->constants.empty())
      << "Total # of constants: " << logger->Count(logger->constants);

  delete logger;
  BooleanGraph::ClearGateMarks();
}

void BooleanGraph::GatherInformation(const IGatePtr& gate,
                                     GraphLogger* logger) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  logger->Log(gate);
  for (const auto& arg : gate->gate_args()) {
    BooleanGraph::GatherInformation(arg.second, logger);
  }
}

std::ostream& operator<<(std::ostream& os, const ConstantPtr& constant) {
  if (constant->Visited()) return os;
  constant->Visit(1);
  std::string state = constant->state() ? "true" : "false";
  os << "s(H" << constant->index() << ") = " << state << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const VariablePtr& variable) {
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
/// @param[in] gate  The gate with the formula to be printed.
///
/// @returns The beginning, operator, and end strings for the formula.
const FormulaSig GetFormulaSig(const std::shared_ptr<const IGate>& gate) {
  FormulaSig sig = {"(", "", ")"};  // Defaults for most gate types.

  switch (gate->type()) {
    case kNand:
      sig.begin = "~(";  // Fall-through to AND gate.
    case kAnd:
      sig.op = " & ";
      break;
    case kNor:
      sig.begin = "~(";  // Fall-through to OR gate.
    case kOr:
      sig.op = " | ";
      break;
    case kXor:
      sig.op = " ^ ";
      break;
    case kNot:
      sig.begin = "~(";  // Parentheses are for cases of NOT(NOT Arg).
      break;
    case kNull:
      sig.begin = "";  // No need for the parentheses.
      sig.end = "";
      break;
    case kVote:
      sig.begin = "@(" + std::to_string(gate->vote_number()) + ", [";
      sig.op = ", ";
      sig.end = "])";
      break;
  }
  return sig;
}

/// Provides special formatting for indexed gate names.
///
/// @param[in] gate  The gate which name must be created.
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

std::ostream& operator<<(std::ostream& os, const IGatePtr& gate) {
  if (gate->Visited()) return os;
  gate->Visit(1);
  std::string name = GetName(gate);
  if (gate->IsConstant()) {
    std::string state = gate->state() == kNullState ? "false" : "true";
    os << "s(" << name << ") = " << state << std::endl;
    return os;
  }
  std::string formula = "";  // The formula of the gate for printing.
  const FormulaSig sig = GetFormulaSig(gate);  // Formatting for the formula.
  int num_args = gate->args().size();  // The number of arguments to print.

  for (const auto& node : gate->gate_args()) {
    if (node.first < 0) formula += "~";  // Negation.
    formula += GetName(node.second);

    if (--num_args) formula += sig.op;

    os << node.second;
  }

  for (const auto& basic : gate->variable_args()) {
    if (basic.first < 0) formula += "~";  // Negation.
    int index = basic.second->index();
    formula += "B" + std::to_string(index);

    if (--num_args) formula += sig.op;

    os << basic.second;
  }

  for (const auto& constant : gate->constant_args()) {
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
