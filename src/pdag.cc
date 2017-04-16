/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file pdag.cc
/// Implementation of a Propositional Directed Acyclic Graph
/// with indexed nodes, variables, and gates.
/// The implementation caters other algorithms like preprocessing.
/// The main goal is
/// to make manipulations and transformations of the graph
/// easier to achieve for graph algorithms.

#include "pdag.h"

#include <iostream>
#include <string>
#include <unordered_set>

#include <boost/math/special_functions/sign.hpp>
#include <boost/range/algorithm.hpp>

#include "event.h"
#include "logger.h"

namespace scram {
namespace core {

void NodeParentManager::AddParent(const GatePtr& gate) {
  assert(!parents_.count(gate->index()) && "Adding an existing parent.");
  parents_.data().emplace_back(gate->index(), gate);
}

Node::Node(Pdag* graph) noexcept
    : index_(Pdag::NodeIndexGenerator()(graph)),
      order_(0),
      visits_{},
      opti_value_(0),
      pos_count_(0),
      neg_count_(0),
      graph_(*graph) {}

Node::~Node() = default;

Gate::Gate(Operator type, Pdag* graph) noexcept
    : Node(graph),
      type_(type),
      mark_(false),
      module_(false),
      coherent_(false),
      vote_number_(0),
      descendant_(0),
      ancestor_(0),
      min_time_(0),
      max_time_(0) {}

void Gate::type(Operator type) {  // Don't use in Gate constructor!
  /// @todo Find the inefficient resets.
  /* assert(type_ != type && "Attribute reset: Operation with no effect."); */
  type_ = type;
  if (type_ == kNull)
    Pdag::NullGateRegistrar()(shared_from_this());
}

GatePtr Gate::Clone() noexcept {
  BLOG(DEBUG5, module_) << "WARNING: Cloning module G" << Node::index();
  assert(!constant() && type_ != kNull);
  auto clone = std::make_shared<Gate>(type_, &Node::graph());  // The same type.
  clone->coherent_ = coherent_;
  clone->vote_number_ = vote_number_;  // Copy vote number in case it is K/N.
  // Getting arguments copied.
  clone->args_ = args_;
  clone->gate_args_ = gate_args_;
  clone->variable_args_ = variable_args_;
  clone->constant_ = constant_;
  // Introducing the new parent to the args.
  for (const auto& arg : gate_args_)
    arg.second->AddParent(clone);
  for (const auto& arg : variable_args_)
    arg.second->AddParent(clone);
  if (constant_)
    constant_->AddParent(clone);
  return clone;
}

/// Specialization to handle True constant arg addition.
template <>
void Gate::AddConstantArg<true>() noexcept {
  switch (type_) {
    case kNull:
    case kOr:
      MakeConstant(true);
      break;
    case kNand:
      ReduceLogic(kNot);
      break;
    case kAnd:
      ReduceLogic(kNull);
      break;
    case kNor:
    case kNot:
      MakeConstant(false);
      break;
    case kXor:  // Special handling due to its internal negation.
      assert(args_.size() == 1);
      type(kNot);
      break;
    case kVote:  // (K - 1) / (N - 1).
      assert(args_.size() >= 2);
      assert(vote_number_ > 0);
      --vote_number_;
      if (vote_number_ == 1)
        type(kOr);
      break;
  }
}

/// Specialization to handle False constant arg addition.
template <>
void Gate::AddConstantArg<false>() noexcept {
  switch (type_) {
    case kNull:
    case kAnd:
      MakeConstant(false);
      break;
    case kNand:
    case kNot:
      MakeConstant(true);
      break;
    case kNor:
      ReduceLogic(kNot);
      break;
    case kOr:
      ReduceLogic(kNull);
      break;
    case kXor:
      assert(args_.size() == 1);
      type(kNull);
      break;
    case kVote:  // K / (N - 1).
      assert(args_.size() >= 2);
      ReduceLogic(kAnd, vote_number_);
      break;
  }
}

/// Convenient wrapper to dispatch appropriate constant arg handler.
template <>
void Gate::AddArg<Constant>(int index, const ConstantPtr& arg) noexcept {
  assert(!constant_);
  assert(arg->value());
  return index > 0 ? AddConstantArg<true>() : AddConstantArg<false>();
}

void Gate::TransferArg(int index, const GatePtr& recipient) noexcept {
  assert(!constant() && "Improper use case.");
  assert(index != 0);
  assert(args_.count(index));
  args_.erase(index);

  if (auto it_g = ext::find(gate_args_, index)) {
    it_g->second->EraseParent(Node::index());
    recipient->AddArg(*it_g);
    gate_args_.erase(it_g);

  } else {
    auto it_v = variable_args_.find(index);
    it_v->second->EraseParent(Node::index());
    recipient->AddArg(*it_v);
    variable_args_.erase(it_v);
  }
}

void Gate::ShareArg(int index, const GatePtr& recipient) noexcept {
  assert(!constant() && "Improper use case.");
  assert(index != 0);
  assert(args_.count(index));
  if (auto it_g = ext::find(gate_args_, index)) {
    recipient->AddArg(*it_g);
  } else {
    recipient->AddArg(*variable_args_.find(index));
  }
}

void Gate::NegateArgs() noexcept {
  /* assert(!constant() && "Improper use case."); */
  /// @todo Consider in place inversion.
  ArgSet inverted_args;
  for (auto it = args_.rbegin(); it != args_.rend(); ++it)
    inverted_args.insert(inverted_args.end(), -*it);
  args_ = std::move(inverted_args);

  for (auto& arg : gate_args_)
    arg.first *= -1;
  for (auto& arg : variable_args_)
    arg.first *= -1;
}

void Gate::NegateArg(int existing_arg) noexcept {
  assert(!constant() && "Improper use case.");
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
  assert(!constant() && "Improper use case.");
  assert(args_.count(arg_gate->index()) && "Cannot join complement gate.");
  assert(!arg_gate->constant() && "Impossible to join.");
  assert(!arg_gate->args().empty() && "Corrupted gate.");

  for (const auto& arg : arg_gate->gate_args_) {
    AddArg(arg);
    if (constant())
      return;
  }
  for (const auto& arg : arg_gate->variable_args_) {
    AddArg(arg);
    if (constant())
      return;
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
    AddArg(arg_index, null_gate->gate_args_.begin()->second);
  } else if (!null_gate->variable_args_.empty()) {
    AddArg(arg_index, null_gate->variable_args_.begin()->second);
  } else {
    assert(null_gate->constant_);
    AddArg(arg_index, null_gate->constant_);
  }
}

void Gate::ProcessConstantArg(const NodePtr& arg, bool state) noexcept {
  int index = GetArgSign(arg) * arg->index();
  if (index < 0)
    state = !state;
  EraseArg(index);
  return state ? AddConstantArg<true>() : AddConstantArg<false>();
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
    assert(constant_);
    constant_->EraseParent(Node::index());
    constant_ = nullptr;
  }
}

void Gate::EraseArgs() noexcept {
  args_.clear();
  for (const auto& arg : gate_args_)
    arg.second->EraseParent(Node::index());
  gate_args_.clear();

  for (const auto& arg : variable_args_)
    arg.second->EraseParent(Node::index());
  variable_args_.clear();

  if (constant_)
    constant_->EraseParent(Node::index());
  constant_ = nullptr;
}

void Gate::MakeConstant(bool state) noexcept {
  assert(!constant());
  EraseArgs();
  type(kNull);
  constant_ = Node::graph().constant();
  int index = state ? constant_->index() : -constant_->index();
  args_.insert(index);
  constant_->AddParent(shared_from_this());
}

void Gate::ProcessDuplicateArg(int index) noexcept {
  assert(type_ != kNot && type_ != kNull);
  assert(args_.count(index));
  LOG(DEBUG5) << "Handling duplicate argument for G" << Node::index();
  if (type_ == kVote)
    return ProcessVoteGateDuplicateArg(index);

  if (args_.size() == 1) {
    LOG(DEBUG5) << "Handling the case of one-arg duplicate argument!";
    switch (type_) {
      case kAnd:
      case kOr:
        type(kNull);
        break;
      case kNand:
      case kNor:
        type(kNot);
        break;
      case kXor:
        LOG(DEBUG5) << "Handling special case of XOR duplicate argument!";
        MakeConstant(false);
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
    this->type(kNull);
    return;
  }
  if (vote_number_ == args_.size()) {  // @(k, [y_i]) is NULL set.
    assert(vote_number_ > 2 && "Corrupted number of gate arguments.");
    GatePtr clone_two = this->Clone();
    clone_two->vote_number(vote_number_ - 2);  // @(k-2, [y_i])
    this->EraseArgs();
    this->type(kAnd);
    clone_two->TransferArg(index, shared_from_this());  // Transferred the x.
    if (clone_two->vote_number() == 1)
      clone_two->type(kOr);
    this->AddArg(clone_two);
    return;
  }
  assert(args_.size() > 2);
  GatePtr clone_one = this->Clone();  // @(k, [y_i])

  this->EraseArgs();  // The main gate turns into OR with x.
  type(kOr);
  this->AddArg(clone_one);
  if (vote_number_ == 2) {  // No need for the second K/N gate.
    clone_one->TransferArg(index, shared_from_this());  // Transferred the x.
    assert(this->args_.size() == 2);
  } else {
    // Create the AND gate to combine with the duplicate node.
    auto and_gate = std::make_shared<Gate>(kAnd, &Node::graph());
    this->AddArg(and_gate);
    clone_one->TransferArg(index, and_gate);  // Transferred the x.

    // Have to create the second K/N for vote_number > 2.
    GatePtr clone_two = clone_one->Clone();
    clone_two->vote_number(vote_number_ - 2);  // @(k-2, [y_i])
    if (clone_two->vote_number() == 1)
      clone_two->type(kOr);
    and_gate->AddArg(clone_two);

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
      MakeConstant(false);
      break;
    case kNand:
    case kXor:
    case kOr:
      MakeConstant(true);
      break;
    case kVote:
      LOG(DEBUG5) << "Handling special case of K/N complement argument!";
      assert(vote_number_ > 1 && "Vote number is wrong.");
      assert((args_.size() + 1) > vote_number_ && "Malformed K/N gate.");
      // @(k, [x, x', y_i]) = @(k-1, [y_i])
      EraseArg(-index);
      --vote_number_;
      if (args_.size() == 1) {
        type(kNull);
      } else if (vote_number_ == 1) {
        type(kOr);
      } else if (vote_number_ == args_.size()) {
        type(kAnd);
      }
      break;
    default:
      assert(false && "Unexpected gate type for complement arg processing.");
  }
}

Pdag::Pdag() noexcept
    : node_index_(0),
      complement_(false),
      coherent_(true),
      normal_(true),
      register_null_gates_(true),
      constant_(new Constant(this)) {}

Pdag::Pdag(const mef::Gate& root, bool ccf) noexcept : Pdag() {
  TIMER(DEBUG2, "PDAG Construction");
  ProcessedNodes nodes;
  GatherVariables(root.formula(), ccf, &nodes);
  root_ = ConstructGate(root.formula(), ccf, &nodes);
}

void Pdag::Print() {
  Clear<kVisit>();
  std::cerr << "\n" << this << std::endl;
}

namespace {
/// Compares operator enums from mef::Operator and core::Operator
#define OPERATOR_EQ(op) static_cast<int>(op) == static_cast<int>(mef::op)

/// @returns true if mef::Operator enum maps exactly to core::Operator enum.
constexpr bool CheckOperatorEnums() {
  return OPERATOR_EQ(kAnd) && OPERATOR_EQ(kOr) && OPERATOR_EQ(kVote) &&
         OPERATOR_EQ(kXor) && OPERATOR_EQ(kNot) && OPERATOR_EQ(kNand) &&
         OPERATOR_EQ(kNor) && OPERATOR_EQ(kNull);
}
#undef OPERATOR_EQ
}  // namespace

void Pdag::GatherVariables(const mef::Formula& formula, bool ccf,
                           ProcessedNodes* nodes) noexcept {
  struct {
    void operator()(const mef::BasicEvent* arg) {
      graph->GatherVariables(*arg, ccf, nodes);
    }
    void operator()(const mef::Gate* mef_gate) {
      if (nodes->gates.emplace(mef_gate, nullptr).second) {
        graph->GatherVariables(mef_gate->formula(), ccf, nodes);
      }
    }
    void operator()(const mef::HouseEvent*) {}

    Pdag* graph;
    bool ccf;
    ProcessedNodes* nodes;
  } formula_visitor{this, ccf, nodes};

  for (const mef::Formula::EventArg& event_arg : formula.event_args()) {
    boost::apply_visitor(formula_visitor, event_arg);
  }

  for (const mef::FormulaPtr& sub_form : formula.formula_args()) {
    GatherVariables(*sub_form, ccf, nodes);
  }
}

void Pdag::GatherVariables(const mef::BasicEvent& basic_event, bool ccf,
                           ProcessedNodes* nodes) noexcept {
  if (ccf && basic_event.HasCcf()) {  // Gather CCF events.
    if (nodes->gates.emplace(&basic_event.ccf_gate(), nullptr).second)
      GatherVariables(basic_event.ccf_gate().formula(), ccf, nodes);
  } else {
    VariablePtr& var = nodes->variables[&basic_event];
    if (!var) {
      basic_events_.push_back(&basic_event);
      var = std::make_shared<Variable>(this);  // Sequential indices.
      assert((kVariableStartIndex + basic_events_.size() - 1) == var->index());
    }
  }
}

/// Specialization for HouseEvent arguments.
template <>
void Pdag::AddArg(const GatePtr& parent, const mef::HouseEvent& house_event,
                  bool /*ccf*/, ProcessedNodes* /*nodes*/) noexcept {
  // Create unique pass-through gates to hold the construction invariant.
  auto null_gate = std::make_shared<Gate>(kNull, this);
  null_gate->AddArg(constant_, !house_event.state());
  parent->AddArg(null_gate);
  null_gates_.push_back(null_gate);
}

/// Specialization for Gate arguments.
template <>
void Pdag::AddArg(const GatePtr& parent, const mef::Gate& gate, bool ccf,
                  ProcessedNodes* nodes) noexcept {
  GatePtr& pdag_gate = nodes->gates.find(&gate)->second;
  if (!pdag_gate) {
    pdag_gate = ConstructGate(gate.formula(), ccf, nodes);
  }
  parent->AddArg(pdag_gate);
}

/// Specialization for BasicEvent arguments.
template <>
void Pdag::AddArg(const GatePtr& parent, const mef::BasicEvent& basic_event,
                  bool ccf, ProcessedNodes* nodes) noexcept {
  if (ccf && basic_event.HasCcf()) {  // Replace with a CCF gate.
    AddArg(parent, basic_event.ccf_gate(), ccf, nodes);
  } else {
    VariablePtr& var = nodes->variables.find(&basic_event)->second;
    assert(var && "Uninitialized variable.");
    parent->AddArg(var);
  }
}

GatePtr Pdag::ConstructGate(const mef::Formula& formula, bool ccf,
                            ProcessedNodes* nodes) noexcept {
  static_assert(kNumOperators == 8, "Unspecified formula operators.");
  static_assert(kNumOperators == mef::kNumOperators, "Operator mismatch.");
  static_assert(CheckOperatorEnums(), "mef::Operator doesn't map to Operator.");

  Operator type = static_cast<Operator>(formula.type());
  auto parent = std::make_shared<Gate>(type, this);

  if (type != kOr && type != kAnd)
    normal_ = false;

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
  for (const mef::Formula::EventArg& event_arg : formula.event_args()) {
    boost::apply_visitor(
        [&](const auto* event) { this->AddArg(parent, *event, ccf, nodes); },
        event_arg);
  }

  for (const mef::FormulaPtr& sub_form : formula.formula_args()) {
    GatePtr new_gate = ConstructGate(*sub_form, ccf, nodes);
    parent->AddArg(new_gate);
  }
  return parent;
}

bool Pdag::IsTrivial() noexcept {
  assert(root_.use_count() == 1 && "Graph gate pointers outside of the graph!");
  /// @todo Enable the code by decouple the order assignment!
  /* if (static_cast<const Pdag*>(this)->IsTrivial()) */
  /*   return true; */
  if (root_->type() != kNull)
    return false;

  RemoveNullGates();  // Ensure that the root is the only pass-through gate.

  LOG(DEBUG3) << "The root NULL gate is processed!";
  assert(root_->args().size() == 1);
  if (!root_->args<Gate>().empty()) {  // Pull the child gate to the root.
    int signed_index = root_->args<Gate>().begin()->first;
    root_ = root_->args<Gate>().begin()->second;  // Destroy the previous root.
    assert(root_->parents().empty() && !root_->constant() &&
           root_->type() != kNull);
    complement() ^= signed_index < 0;
    return false;
  }
  // Only one variable/constant argument.
  LOG(DEBUG4) << "The root NULL gate has only single variable!";
  if (complement()) {
    root_->NegateArgs();
    complement() = false;
  }
  BLOG(DEBUG3, root_->constant()) << "The root gate has become constant!";
  if (!root_->constant()) {
    assert(root_->args<Variable>().size() == 1);
    /// @todo Decouple the order assignment!
    root_->args<Variable>().begin()->second->order(1);
  }
  assert(static_cast<const Pdag*>(this)->IsTrivial());
  return true;
}

void Pdag::RemoveNullGates() noexcept {
  BLOG(DEBUG5, HasConstants()) << "Got CONST gates to clear!";
  BLOG(DEBUG5, HasNullGates()) << "Got NULL gates to clear!";
  Clear<kGateMark>();  // New gates may get created without marks!
  register_null_gates_ = false;
  for (const GateWeakPtr& ptr : null_gates_) {
    if (ptr.expired())
      continue;
    PropagateNullGate(ptr.lock());
  }
  null_gates_.clear();
  register_null_gates_ = true;
  assert(root()->constant() || !HasConstants());
  assert(root()->type() == kNull || !HasNullGates());
}

void Pdag::PropagateNullGate(const GatePtr& gate) noexcept {
  assert(gate->type() == kNull);
  while (!gate->parents().empty()) {
    GatePtr parent = gate->parents().begin()->second.lock();
    int sign = parent->GetArgSign(gate);
    parent->JoinNullGate(sign * gate->index());
    if (parent->type() == kNull) {
      PropagateNullGate(parent);
    }
  }
}

namespace {  // Helper facilities to log the PDAG.

/// Container for properties of PDAGs.
struct GraphLogger {
  /// Special handling of the root gate
  /// because it doesn't have parents.
  ///
  /// @param[in] gate  The root gate of the PDAG.
  explicit GraphLogger(const GatePtr& gate) noexcept {
    gates.insert(gate->index());
  }

  /// Traverses a PDAG to collect information.
  ///
  /// @param[in] gate  The starting gate for traversal.
  void GatherInformation(const GatePtr& gate) noexcept {
    TraverseGates(gate, [this](const GatePtr& node) { Log(node); });
  }

  /// Collects data from a gate.
  ///
  /// @param[in] gate  Valid gate with arguments.
  ///
  /// @pre The gate has not been passed before.
  void Log(const GatePtr& gate) noexcept {
    ++gate_types[gate->type()];
    if (gate->module())
      ++num_modules;
    for (const auto& arg : gate->args<Gate>())
      gates.insert(arg.first);
    for (const auto& arg : gate->args<Variable>())
      variables.insert(arg.first);
  }

  /// @param[in] container  Collection of indices of elements.
  ///
  /// @returns The total number of unique elements.
  int Count(const std::unordered_set<int>& container) noexcept {
    return boost::count_if(container, [&container](int index) {
      return index > 0 || !container.count(-index);
    });
  }

  /// @param[in] container  Collection of indices of elements.
  ///
  /// @returns The total number of complement elements.
  int CountComplements(const std::unordered_set<int>& container) noexcept {
    return boost::count_if(container, [](int index) { return index < 0; });
  }

  /// @param[in] container  Collection of indices of elements.
  ///
  /// @returns The number of literals appearing as positive and negative.
  int CountOverlap(const std::unordered_set<int>& container) noexcept {
    return boost::count_if(container, [&container](int index) {
      return index < 0 && container.count(-index);
    });
  }

  int num_modules = 0;  ///< The number of module gates.
  std::unordered_set<int> gates;  ///< Collection of gates.
  std::array<int, kNumOperators> gate_types{};  ///< Gate type counts.
  std::unordered_set<int> variables;  ///< Collection of variables.
};

}  // namespace

void Pdag::Log() noexcept {
  if (DEBUG4 > scram::Logger::report_level())
    return;
  Clear<kGateMark>();
  GraphLogger logger(root_);
  logger.GatherInformation(root_);
  Clear<kGateMark>();
  LOG(DEBUG4) << "PDAG with root G" << root_->index();
  LOG(DEBUG4) << "Total # of gates: " << logger.Count(logger.gates);
  LOG(DEBUG4) << "# of modules: " << logger.num_modules;
  LOG(DEBUG4) << "# of gates with negative indices: "
              << logger.CountComplements(logger.gates);
  LOG(DEBUG4) << "# of gates with positive and negative indices: "
              << logger.CountOverlap(logger.gates);

  BLOG(DEBUG5, logger.gate_types[kAnd]) << "AND gates: "
                                        << logger.gate_types[kAnd];
  BLOG(DEBUG5, logger.gate_types[kOr]) << "OR gates: "
                                       << logger.gate_types[kOr];
  BLOG(DEBUG5, logger.gate_types[kVote]) << "K/N gates: "
                                         << logger.gate_types[kVote];
  BLOG(DEBUG5, logger.gate_types[kXor]) << "XOR gates: "
                                        << logger.gate_types[kXor];
  BLOG(DEBUG5, logger.gate_types[kNot]) << "NOT gates: "
                                        << logger.gate_types[kNot];
  BLOG(DEBUG5, logger.gate_types[kNand]) << "NAND gates: "
                                         << logger.gate_types[kNand];
  BLOG(DEBUG5, logger.gate_types[kNor]) << "NOR gates: "
                                        << logger.gate_types[kNor];
  BLOG(DEBUG5, logger.gate_types[kNull]) << "NULL gates: "
                                         << logger.gate_types[kNull];

  LOG(DEBUG4) << "Total # of variables: " << logger.Count(logger.variables);
  LOG(DEBUG4) << "# of variables with negative indices: "
              << logger.CountComplements(logger.variables);
  LOG(DEBUG4) << "# of variables with positive and negative indices: "
              << logger.CountOverlap(logger.variables);

  BLOG(DEBUG4, !constant_->parents().empty()) << "Total # of constants: "
                                              << constant_->parents().size();
}

std::ostream& operator<<(std::ostream& os, const Constant& constant) {
  os << "s(H" << constant.index() << ") = "
     << (constant.value() ? "true" : "false") << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Variable& variable) {
  os << "p(B" << variable.index() << ") = " << 1 << "\n";
  return os;
}

namespace {

/// Gate formula signature for printing in the Aralia format.
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
  if (gate.constant()) {
    name += "C";
  } else if (gate.module()) {
    name += "M";
  }
  name += std::to_string(gate.index());
  return name;
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const GatePtr& gate) {
  if (gate->Visited())
    return os;
  gate->Visit(1);
  std::string formula;  // The formula of the gate for printing.
  const FormulaSig sig = GetFormulaSig(*gate);  // Formatting for the formula.
  int num_args = gate->args().size();  // The number of arguments to print.

  auto print_arg = [&os, &formula, &sig, &num_args](int index,
                                                    const std::string& name) {
    if (index < 0)
      formula += "~";  // Negation.
    formula += name;
    if (--num_args)
      formula += sig.op;
  };

  for (const auto& node : gate->args<Gate>()) {
    print_arg(node.first, GetName(*node.second));
    os << node.second;
  }

  for (const auto& basic : gate->args<Variable>()) {
    print_arg(basic.first, "B" + std::to_string(basic.second->index()));
    if (!basic.second->Visited()) {
      basic.second->Visit(1);
      os << *basic.second;
    }
  }

  if (gate->constant()) {
    assert(gate->type() == kNull);
    int index = *gate->args().begin();
    print_arg(index, "H" + std::to_string(std::abs(index)));
  }
  os << GetName(*gate) << " := " << sig.begin << formula << sig.end << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, Pdag* graph) {
  os << "PDAG" << "\n\n" << graph->root();
  if (!graph->constant()->parents().empty())
    os << *graph->constant();
  return os;
}

}  // namespace core
}  // namespace scram
