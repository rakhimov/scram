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

#include <boost/math/special_functions/sign.hpp>

#include "logger.h"

namespace scram {
namespace core {

int Node::next_index_ = 1e6;  // Limit for basic events per fault tree!

void NodeParentManager::AddParent(const GatePtr& gate) {
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

Gate::Gate(Operator type) noexcept
    : type_(type),
      state_(kNormalState),
      vote_number_(0),
      descendant_(0),
      ancestor_(0),
      min_time_(0),
      max_time_(0),
      mark_(false),
      module_(false),
      coherent_(false) {}

GatePtr Gate::Clone() noexcept {
  BLOG(DEBUG5, module_) << "WARNING: Cloning module G" << Node::index();
  auto clone = std::make_shared<Gate>(type_);  // The same type.
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

void Gate::TransferArg(int index, const GatePtr& recipient) noexcept {
  assert(constant_args_.empty() && "Improper use case.");
  assert(index != 0);
  assert(args_.count(index));
  args_.erase(index);

  if (auto it_g = ext::find(gate_args_, index)) {
    it_g->second->EraseParent(Node::index());
    recipient->AddArg(index, it_g->second);
    gate_args_.erase(it_g);

  } else {
    auto it_v = variable_args_.find(index);
    it_v->second->EraseParent(Node::index());
    recipient->AddArg(index, it_v->second);
    variable_args_.erase(it_v);
  }
}

void Gate::ShareArg(int index, const GatePtr& recipient) noexcept {
  assert(constant_args_.empty() && "Improper use case.");
  assert(index != 0);
  assert(args_.count(index));
  if (auto it_g = ext::find(gate_args_, index)) {
    recipient->AddArg(index, it_g->second);
  } else {
    recipient->AddArg(index, variable_args_.find(index)->second);
  }
}

void Gate::InvertArgs() noexcept {
  assert(constant_args_.empty() && "Improper use case.");
  ArgSet inverted_args;
  for (auto it = args_.rbegin(); it != args_.rend(); ++it)
    inverted_args.insert(inverted_args.end(), -*it);
  args_ = std::move(inverted_args);

  for (auto& arg : gate_args_) arg.first *= -1;
  for (auto& arg : variable_args_) arg.first *= -1;
}

void Gate::InvertArg(int existing_arg) noexcept {
  assert(constant_args_.empty() && "Improper use case.");
  assert(args_.count(existing_arg));
  assert(!args_.count(-existing_arg));

  args_.erase(existing_arg);
  args_.insert(-existing_arg);

  if (auto it_g = ext::find(gate_args_, existing_arg)) {
    it_g->first *= -1;

  } else {
    auto it_v = variable_args_.find(existing_arg);
    it_v->first *= -1;
  }
}

void Gate::CoalesceGate(const GatePtr& arg_gate) noexcept {
  assert(constant_args_.empty() && "Improper use case.");
  assert(args_.count(arg_gate->index()) && "Cannot join complement gate.");
  assert(arg_gate->state() == kNormalState && "Impossible to join.");
  assert(!arg_gate->args().empty() && "Corrupted gate.");

  for (const auto& arg : arg_gate->gate_args_) {
    Gate::AddArg(arg.first, arg.second);
    if (state_ != kNormalState) return;
  }
  for (const auto& arg : arg_gate->variable_args_) {
    Gate::AddArg(arg.first, arg.second);
    if (state_ != kNormalState) return;
  }

  args_.erase(arg_gate->index());  // Erase at the end to avoid the type change.
  gate_args_.erase(arg_gate->index());
  arg_gate->EraseParent(Node::index());
}

void Gate::JoinNullGate(int index) noexcept {
  assert(index != 0);
  assert(args_.count(index));
  assert(gate_args_.count(index));

  args_.erase(index);
  auto it_g = gate_args_.find(index);
  GatePtr null_gate = it_g->second;
  gate_args_.erase(it_g);
  null_gate->EraseParent(Node::index());

  assert(null_gate->type_ == kNull);
  assert(null_gate->args_.size() == 1);

  int arg_index = *null_gate->args_.begin();
  arg_index *= boost::math::sign(index);  // Carry the parent's sign.

  if (!null_gate->gate_args_.empty()) {
    Gate::AddArg(arg_index, null_gate->gate_args_.begin()->second);
  } else if (!null_gate->variable_args_.empty()) {
    Gate::AddArg(arg_index, null_gate->variable_args_.begin()->second);
  } else {
    assert(!null_gate->constant_args_.empty());
    Gate::AddArg(arg_index, null_gate->constant_args_.begin()->second);
  }
}

void Gate::ProcessConstantArg(const NodePtr& arg, bool state) noexcept {
  int index = Gate::GetArgSign(arg) * arg->index();
  if (index < 0) state = !state;
  if (state) {  // Unity state or True index.
    Gate::ProcessTrueArg(index);
  } else {  // Null state or False index.
    Gate::ProcessFalseArg(index);
  }
}

void Gate::ProcessTrueArg(int index) noexcept {
  switch (type_) {
    case kNull:
    case kOr:
      Gate::MakeUnity();
      break;
    case kNand:
    case kAnd:
      Gate::RemoveConstantArg(index);
      break;
    case kNor:
    case kNot:
      Gate::Nullify();
      break;
    case kXor:  // Special handling due to its internal negation.
      assert(args_.size() == 2);
      Gate::EraseArg(index);
      assert(args_.size() == 1);
      type_ = kNot;
      break;
    case kVote:  // (K - 1) / (N - 1).
      assert(args_.size() > 2);
      Gate::EraseArg(index);
      assert(vote_number_ > 0);
      --vote_number_;
      if (vote_number_ == 1) type_ = kOr;
      break;
  }
}

void Gate::ProcessFalseArg(int index) noexcept {
  switch (type_) {
    case kNor:
    case kXor:
    case kOr:
      Gate::RemoveConstantArg(index);
      break;
    case kNull:
    case kAnd:
      Gate::Nullify();
      break;
    case kNand:
    case kNot:
      Gate::MakeUnity();
      break;
    case kVote:  // K / (N - 1).
      assert(args_.size() > 2);
      Gate::EraseArg(index);
      if (vote_number_ == args_.size()) type_ = kAnd;
      break;
  }
}

void Gate::RemoveConstantArg(int index) noexcept {
  assert(args_.size() > 1 && "One-arg gate must have become constant.");
  Gate::EraseArg(index);
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

void Gate::EraseArg(int index) noexcept {
  assert(index != 0);
  assert(args_.count(index));
  args_.erase(index);

  if (auto it_g = ext::find(gate_args_, index)) {
    it_g->second->EraseParent(Node::index());
    gate_args_.erase(it_g);

  } else if (auto it_v = ext::find(variable_args_, index)) {
    it_v->second->EraseParent(Node::index());
    variable_args_.erase(it_v);

  } else {
    auto it_c = constant_args_.find(index);
    it_c->second->EraseParent(Node::index());
    constant_args_.erase(it_c);
  }
}

void Gate::EraseAllArgs() noexcept {
  args_.clear();
  for (const auto& arg : gate_args_) arg.second->EraseParent(Node::index());
  gate_args_.clear();
  for (const auto& arg : variable_args_) arg.second->EraseParent(Node::index());
  variable_args_.clear();
  for (const auto& arg : constant_args_) arg.second->EraseParent(Node::index());
  constant_args_.clear();
}

void Gate::ProcessDuplicateArg(int index) noexcept {
  assert(type_ != kNot && type_ != kNull);
  assert(args_.count(index));
  LOG(DEBUG5) << "Handling duplicate argument for G" << Node::index();
  if (type_ == kVote)
    return Gate::ProcessVoteGateDuplicateArg(index);

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
        Gate::Nullify();
        break;
      default:
        assert(false && "NOT and NULL gates can't have duplicates.");
    }
  }
}

void Gate::ProcessVoteGateDuplicateArg(int index) noexcept {
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
    GatePtr clone_two = this->Clone();
    clone_two->vote_number(vote_number_ - 2);  // @(k-2, [y_i])
    this->EraseAllArgs();
    this->type_ = kAnd;
    clone_two->TransferArg(index, shared_from_this());  // Transfered the x.
    if (clone_two->vote_number() == 1) clone_two->type(kOr);
    this->AddArg(clone_two->index(), clone_two);
    return;
  }
  assert(args_.size() > 2);
  GatePtr clone_one = this->Clone();  // @(k, [y_i])

  this->EraseAllArgs();  // The main gate turns into OR with x.
  type_ = kOr;
  this->AddArg(clone_one->index(), clone_one);
  if (vote_number_ == 2) {  // No need for the second K/N gate.
    clone_one->TransferArg(index, shared_from_this());  // Transfered the x.
    assert(this->args_.size() == 2);
  } else {
    // Create the AND gate to combine with the duplicate node.
    auto and_gate = std::make_shared<Gate>(kAnd);
    this->AddArg(and_gate->index(), and_gate);
    clone_one->TransferArg(index, and_gate);  // Transfered the x.

    // Have to create the second K/N for vote_number > 2.
    GatePtr clone_two = clone_one->Clone();
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

void Gate::ProcessComplementArg(int index) noexcept {
  assert(type_ != kNot && type_ != kNull);
  assert(args_.count(-index));
  LOG(DEBUG5) << "Handling complement argument for G" << Node::index();
  switch (type_) {
    case kNor:
    case kAnd:
      Gate::Nullify();
      break;
    case kNand:
    case kXor:
    case kOr:
      Gate::MakeUnity();
      break;
    case kVote:
      LOG(DEBUG5) << "Handling special case of K/N complement argument!";
      assert(vote_number_ > 1 && "Vote number is wrong.");
      assert((args_.size() + 1) > vote_number_ && "Malformed K/N gate.");
      // @(k, [x, x', y_i]) = @(k-1, [y_i])
      Gate::EraseArg(-index);
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

const std::unordered_map<std::string, Operator> BooleanGraph::kStringToType_ = {
    {"and", kAnd},
    {"or", kOr},
    {"atleast", kVote},
    {"xor", kXor},
    {"not", kNot},
    {"nand", kNand},
    {"nor", kNor},
    {"null", kNull}};

BooleanGraph::BooleanGraph(const mef::Gate& root, bool ccf) noexcept
    : root_sign_(1),
      coherent_(true),
      normal_(true) {
  Node::ResetIndex();
  Variable::ResetIndex();
  ProcessedNodes nodes;
  root_ = BooleanGraph::ProcessFormula(root.formula(), ccf, &nodes);
}

void BooleanGraph::Print() {
  ClearNodeVisits();
  std::cerr << "\n" << this << std::endl;
}

GatePtr BooleanGraph::ProcessFormula(const mef::Formula& formula, bool ccf,
                                     ProcessedNodes* nodes) noexcept {
  Operator type = kStringToType_.find(formula.type())->second;
  auto parent = std::make_shared<Gate>(type);

  if (type != kOr && type != kAnd) normal_ = false;

  switch (type) {
    case kNot:
    case kNand:
    case kNor:
    case kXor:
      coherent_ = false;
      break;
    case kVote:
      parent->vote_number(formula.vote_number());
      break;
    case kNull:
      null_gates_.push_back(parent);
      break;
    default:
      assert((type == kOr || type == kAnd) && "Unexpected gate type.");
  }
  for (const mef::BasicEventPtr& basic_event : formula.basic_event_args()) {
    BooleanGraph::ProcessBasicEvent(parent, basic_event.get(), ccf, nodes);
  }

  for (const mef::HouseEventPtr& house_event : formula.house_event_args()) {
    BooleanGraph::ProcessHouseEvent(parent, *house_event, nodes);
  }

  for (const mef::GatePtr& mef_gate : formula.gate_args()) {
    BooleanGraph::ProcessGate(parent, *mef_gate, ccf, nodes);
  }

  for (const mef::FormulaPtr& sub_form : formula.formula_args()) {
    GatePtr new_gate = BooleanGraph::ProcessFormula(*sub_form, ccf, nodes);
    parent->AddArg(new_gate->index(), new_gate);
  }
  return parent;
}

void BooleanGraph::ProcessBasicEvent(const GatePtr& parent,
                                     mef::BasicEvent* basic_event, bool ccf,
                                     ProcessedNodes* nodes) noexcept {
  if (ccf && basic_event->HasCcf()) {  // Replace with a CCF gate.
    GatePtr& ccf_gate = nodes->gates[&basic_event->ccf_gate()];
    if (!ccf_gate) {
      ccf_gate = BooleanGraph::ProcessFormula(basic_event->ccf_gate().formula(),
                                              ccf, nodes);
    }
    parent->AddArg(ccf_gate->index(), ccf_gate);
  } else {
    VariablePtr& var = nodes->variables[basic_event];
    if (!var) {
      basic_events_.push_back(basic_event);
      var = std::make_shared<Variable>();  // Sequential indices.
      assert(basic_events_.size() == var->index());
    }
    parent->AddArg(var->index(), var);
  }
}

void BooleanGraph::ProcessHouseEvent(const GatePtr& parent,
                                     const mef::HouseEvent& house_event,
                                     ProcessedNodes* nodes) noexcept {
  ConstantPtr& constant = nodes->constants[&house_event];
  if (!constant) {
    constant = std::make_shared<Constant>(house_event.state());
    constants_.push_back(constant);
  }
  parent->AddArg(constant->index(), constant);
}

void BooleanGraph::ProcessGate(const GatePtr& parent, const mef::Gate& gate,
                               bool ccf, ProcessedNodes* nodes) noexcept {
  GatePtr& pdag_gate = nodes->gates[&gate];
  if (!pdag_gate) {
    pdag_gate = BooleanGraph::ProcessFormula(gate.formula(), ccf, nodes);
  }
  parent->AddArg(pdag_gate->index(), pdag_gate);
}

void BooleanGraph::ClearGateMarks() noexcept {
  BooleanGraph::ClearGateMarks(root_);
}

void BooleanGraph::ClearGateMarks(const GatePtr& gate) noexcept {
  if (!gate->mark()) return;
  gate->mark(false);
  for (const auto& arg : gate->args<Gate>()) {
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

void BooleanGraph::ClearNodeVisits(const GatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  if (gate->Visited()) gate->ClearVisits();

  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::ClearNodeVisits(arg.second);
  }
  for (const auto& arg : gate->args<Variable>()) {
    if (arg.second->Visited()) arg.second->ClearVisits();
  }
  for (const auto& arg : gate->args<Constant>()) {
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

void BooleanGraph::ClearOptiValues(const GatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  gate->opti_value(0);
  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::ClearOptiValues(arg.second);
  }
  for (const auto& arg : gate->args<Variable>()) {
    arg.second->opti_value(0);
  }
  assert(gate->args<Constant>().empty());
}

void BooleanGraph::ClearNodeCounts() noexcept {
  LOG(DEBUG5) << "Clearing node counts...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearNodeCounts(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node counts are clear!";
}

void BooleanGraph::ClearNodeCounts(const GatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  gate->ResetCount();
  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::ClearNodeCounts(arg.second);
  }
  for (const auto& arg : gate->args<Variable>()) {
    arg.second->ResetCount();
  }
  assert(gate->args<Constant>().empty());
}

void BooleanGraph::ClearDescendantMarks() noexcept {
  LOG(DEBUG5) << "Clearing gate descendant marks...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearDescendantMarks(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Descendant marks are clear!";
}

void BooleanGraph::ClearDescendantMarks(const GatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  gate->descendant(0);
  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::ClearDescendantMarks(arg.second);
  }
}

void BooleanGraph::ClearAncestorMarks() noexcept {
  LOG(DEBUG5) << "Clearing gate descendant marks...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearAncestorMarks(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Descendant marks are clear!";
}

void BooleanGraph::ClearAncestorMarks(const GatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  gate->ancestor(0);
  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::ClearAncestorMarks(arg.second);
  }
}

void BooleanGraph::ClearNodeOrders() noexcept {
  LOG(DEBUG5) << "Clearing node order marks...";
  BooleanGraph::ClearGateMarks();
  BooleanGraph::ClearNodeOrders(root_);
  BooleanGraph::ClearGateMarks();
  LOG(DEBUG5) << "Node order marks are clear!";
}

void BooleanGraph::ClearNodeOrders(const GatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  if (gate->order()) gate->order(0);
  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::ClearNodeOrders(arg.second);
  }
  for (const auto& arg : gate->args<Variable>()) {
    if (arg.second->order()) arg.second->order(0);
  }
  assert(gate->args<Constant>().empty());
}

void BooleanGraph::GraphLogger::RegisterRoot(const GatePtr& gate) noexcept {
  gates.insert(gate->index());
}

void BooleanGraph::GraphLogger::Log(const GatePtr& gate) noexcept {
  ++gate_types[gate->type()];
  if (gate->module()) ++num_modules;
  for (const auto& arg : gate->args<Gate>()) gates.insert(arg.first);
  for (const auto& arg : gate->args<Variable>()) variables.insert(arg.first);
  for (const auto& arg : gate->args<Constant>()) constants.insert(arg.first);
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

void BooleanGraph::GatherInformation(const GatePtr& gate,
                                     GraphLogger* logger) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  logger->Log(gate);
  for (const auto& arg : gate->args<Gate>()) {
    BooleanGraph::GatherInformation(arg.second, logger);
  }
}

std::ostream& operator<<(std::ostream& os, const ConstantPtr& constant) {
  if (constant->Visited()) return os;
  constant->Visit(1);
  std::string state = constant->state() ? "true" : "false";
  os << "s(H" << constant->index() << ") = " << state << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const VariablePtr& variable) {
  if (variable->Visited()) return os;
  variable->Visit(1);
  os << "p(B" << variable->index() << ") = " << 1 << "\n";
  return os;
}

namespace {

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
FormulaSig GetFormulaSig(const Gate& gate) {
  FormulaSig sig = {"(", "", ")"};  // Defaults for most gate types.

  switch (gate.type()) {
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
      sig.begin = "@(" + std::to_string(gate.vote_number()) + ", [";
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
std::string GetName(const Gate& gate) {
  std::string name = "G";
  if (gate.state() == kNormalState) {
    if (gate.module()) name += "M";
  } else {  // This gate has become constant.
    name += "C";
  }
  name += std::to_string(gate.index());
  return name;
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const GatePtr& gate) {
  if (gate->Visited()) return os;
  gate->Visit(1);
  if (gate->IsConstant()) {
    std::string state = gate->state() == kNullState ? "false" : "true";
    os << "s(" << GetName(*gate) << ") = " << state << "\n";
    return os;
  }
  std::string formula;  // The formula of the gate for printing.
  const FormulaSig sig = GetFormulaSig(*gate);  // Formatting for the formula.
  int num_args = gate->args().size();  // The number of arguments to print.

  for (const auto& node : gate->args<Gate>()) {
    if (node.first < 0) formula += "~";  // Negation.
    formula += GetName(*node.second);
    if (--num_args) formula += sig.op;
    os << node.second;
  }

  for (const auto& basic : gate->args<Variable>()) {
    if (basic.first < 0) formula += "~";  // Negation.
    formula += "B" + std::to_string(basic.second->index());
    if (--num_args) formula += sig.op;
    os << basic.second;
  }

  for (const auto& constant : gate->args<Constant>()) {
    if (constant.first < 0) formula += "~";  // Negation.
    formula += "H" + std::to_string(constant.second->index());
    if (--num_args) formula += sig.op;
    os << constant.second;
  }
  os << GetName(*gate) << " := " << sig.begin << formula << sig.end << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const BooleanGraph* ft) {
  os << "BooleanGraph" << "\n\n" << ft->root();
  return os;
}

}  // namespace core
}  // namespace scram
