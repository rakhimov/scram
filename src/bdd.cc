/*
 * Copyright (C) 2015 Olzhas Rakhimov
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

/// @file bdd.cc
/// Implementation of BDD fault tree analysis algorithms.

#include "bdd.h"

#include <algorithm>

#include "event.h"

namespace scram {

Vertex::~Vertex() {}  // Empty body for pure virtual destructor.

Terminal::Terminal(bool value) : value_(value) {}

std::pair<bool, bool> Ite::ApplyIfTerminal(Operator type, const ItePtr& arg_one,
                                           const ItePtr& arg_two) noexcept {
  assert(arg_one && arg_two);
  assert(arg_one->id() && arg_two->id());  // Reduced and ordered.
  assert(arg_one->order() <= arg_two->order());  // Must be passed in order.
  assert(!high_ && !low_ && !high_term_ && !low_term_);  // Must be new.
  if (arg_one->order() == arg_two->order()) {
    high_term_ =
        Ite::ApplyTerminal(type, arg_one->high_term_, arg_two->high_term_);
    low_term_ =
        Ite::ApplyTerminal(type, arg_one->low_term_, arg_two->low_term_);

    Ite::ApplyTerminal(type, arg_one->high_, arg_two->high_term_, &high_,
                       &high_term_);
    Ite::ApplyTerminal(type, arg_two->high_, arg_one->high_term_, &high_,
                       &high_term_);

    Ite::ApplyTerminal(type, arg_one->low_, arg_two->low_term_, &low_,
                       &low_term_);
    Ite::ApplyTerminal(type, arg_two->low_, arg_one->low_term_, &low_,
                       &low_term_);
  } else {
    assert(arg_one->order() < arg_two->order());  // Double check.
    Ite::ApplyTerminal(type, arg_two, arg_one->high_term_, &high_, &high_term_);
    Ite::ApplyTerminal(type, arg_two, arg_one->low_term_, &low_, &low_term_);
  }
  assert(!high_ || !high_term_);
  assert(!low_ || !low_term_);
  bool high = high_term_ || high_;
  bool low = low_term_ || low_;
  return {high, low};
}

std::shared_ptr<Terminal> Ite::ApplyTerminal(
    Operator type,
    const TerminalPtr& term_one,
    const TerminalPtr& term_two) noexcept {
  if (!term_one || !term_two) return nullptr;
  switch (type) {
    case kOrGate:
      return term_one->value() ? term_one : term_two;
    case kAndGate:
      return term_one->value() ? term_two : term_one;  // Reverse of OR logic!
    default:
      assert(false);
  }
}

void Ite::ApplyTerminal(Operator type, const ItePtr& v_one,
                        const TerminalPtr& term_one,
                        ItePtr* branch, TerminalPtr* branch_term) noexcept {
  if (!v_one || !term_one) return;
  assert(!*branch && !*branch_term);
  switch (type) {
    case kOrGate:
      if (term_one->value()) {
        *branch_term = term_one;
      } else {
        *branch = v_one;
      }
      break;
    case kAndGate:
      if (term_one->value()) {
        *branch = v_one;
      } else {
        *branch_term = term_one;
      }
      break;
    default:
      assert(false);
  }
}

Bdd::Bdd(BooleanGraph* fault_tree)
    : fault_tree_(fault_tree),
      p_graph_(0),
      kOne_(std::make_shared<Terminal>(true)),
      kZero_(std::make_shared<Terminal>(false)),
      function_id_(2) {}

void Bdd::Analyze() noexcept {
  assert(fault_tree_->coherent());
  fault_tree_->ClearOptiValues();
  int shift = Bdd::TopologicalOrder(fault_tree_->root(), 0);
  fault_tree_->ClearNodeVisits();
  Bdd::AdjustOrder(fault_tree_->root(), ++shift);
  assert(fault_tree_->root()->opti_value() == 1);
  Bdd::ConvertGate(fault_tree_->root());
  ItePtr root_vertex = gates_.find(fault_tree_->root()->index())->second;
  probs_.reserve(fault_tree_->basic_events().size() + 1);
  probs_.push_back(-1);  // First one is a dummy.
  using BasicEventPtr = std::shared_ptr<BasicEvent>;
  for (const BasicEventPtr& event : fault_tree_->basic_events()) {
    probs_.push_back(event->p());
  }
  p_graph_ = Bdd::CalculateProbability(root_vertex);
}

int Bdd::TopologicalOrder(const IGatePtr& root, int order) noexcept {
  if (root->opti_value()) return order;
  for (const std::pair<int, IGatePtr>& arg : root->gate_args()) {
    order = Bdd::TopologicalOrder(arg.second, order);
  }
  using VariablePtr = std::shared_ptr<Variable>;
  for (const std::pair<int, VariablePtr>& arg : root->variable_args()) {
    if (!arg.second->opti_value()) arg.second->opti_value(++order);
  }
  assert(root->constant_args().empty());
  root->opti_value(++order);
  return order;
}

void Bdd::AdjustOrder(const IGatePtr& root, int shift) noexcept {
  if (root->Visited()) return;
  root->Visit(1);
  root->opti_value(shift - root->opti_value());
  for (const std::pair<int, IGatePtr>& arg : root->gate_args()) {
    Bdd::AdjustOrder(arg.second, shift);
  }
  using VariablePtr = std::shared_ptr<Variable>;
  for (const std::pair<int, VariablePtr>& arg : root->variable_args()) {
    if (arg.second->Visited()) continue;
    arg.second->Visit(1);
    arg.second->opti_value(shift - arg.second->opti_value());
  }
}

std::shared_ptr<Ite> Bdd::ConvertGate(const IGatePtr& gate) noexcept {
  ItePtr& bdd_graph = gates_[gate->index()];
  if (bdd_graph) return bdd_graph;

  if (!gate->variable_args().empty()) {
    // Process variable arguments into a BDD graph.
    std::vector<std::pair<int, NodePtr>> args;
    args.insert(args.end(), gate->variable_args().begin(),
                gate->variable_args().end());
    std::sort(args.begin(), args.end(),
              [](const std::pair<int, NodePtr>& lhs,
                 const std::pair<int, NodePtr>& rhs) {
              return lhs.second->opti_value() < rhs.second->opti_value();
              });
    bdd_graph = Bdd::IfThenElse(gate->type(), args, 0);  // Make ordered.
    ItePtr res = Bdd::ReduceGraph(bdd_graph);  // Make reduced.
    if (res) bdd_graph = res;  // Replace if needed.
  }

  // Apply the gate operator to BDD graphs of argument variables and gates.
  /// @todo Determine if order matters among gates.
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    ItePtr arg_graph = Bdd::ConvertGate(arg.second);
    if (!bdd_graph) {  // No variable arguments have been processed.
      bdd_graph = arg_graph;
      continue;
    }
    bdd_graph = Bdd::Apply(gate->type(), bdd_graph, arg_graph);
  }
  assert(bdd_graph);
  return bdd_graph;
}

std::shared_ptr<Ite> Bdd::ReduceGraph(const ItePtr& ite) noexcept {
  if (!ite) return nullptr;  // Terminal nodes.
  if (ite->id()) return nullptr;  // Already reduced function graph.
  ItePtr high = Bdd::ReduceGraph(ite->high());
  ItePtr low = Bdd::ReduceGraph(ite->low());
  if (high) ite->high(high);
  if (low) ite->low(low);
  int id_high = ite->IdHigh();
  int id_low = ite->IdLow();
  if (id_low == id_high) {  // Redundancy condition.
    assert(ite->low());  // Non-terminal.
    assert(ite->low() == ite->high());
    return ite->low();
  }
  ItePtr& in_table = unique_table_[{ite->index(), id_high, id_low}];
  if (in_table) return in_table;  // Existing function graph.
  ite->id(function_id_++);  // Unique function graph.
  in_table = ite;
  return nullptr;  // No need for replacement.
}

std::shared_ptr<Ite> Bdd::IfThenElse(
    Operator type,
    const std::vector<std::pair<int, NodePtr>>& args,
    int pos) noexcept {
  assert(pos >= 0);
  assert(pos < args.size());
  const auto& arg = args[pos];
  ++pos;  // Next argument for subgraphs.
  assert(arg.first > 0);
  ItePtr ite(new Ite(arg.second->index(), arg.second->opti_value()));
  switch (type) {
    case kOrGate:
      ite->high(kOne_);
      if (pos == args.size()) {
        ite->low(kZero_);
      } else {
        ite->low(Bdd::IfThenElse(type, args, pos));
      }
      break;
    case kAndGate:
      if (pos == args.size()) {
        ite->high(kOne_);
      } else {
        ite->high(Bdd::IfThenElse(type, args, pos));
      }
      ite->low(kZero_);
      break;
    default:
      assert(false);
  }
  return ite;
}

std::shared_ptr<Ite> Bdd::Apply(Operator type, const ItePtr& arg_one,
                                const ItePtr& arg_two) noexcept {
  assert(arg_one && arg_two);
  std::array<int, 3> sig = {0, 0, 0};  // Signature of the operation.
  int min_id = std::min(arg_one->id(), arg_two->id());
  int max_id = std::max(arg_one->id(), arg_two->id());
  assert(min_id && max_id);  // Both are reduced function graphs.
  if (min_id == max_id) return arg_one;
  switch (type) {
    case kOrGate:
      sig[0] = min_id;
      sig[1] = 1;
      sig[2] = max_id;
      break;
    case kAndGate:
      sig[0] = min_id;
      sig[1] = max_id;
      sig[2] = 0;
      break;
    default:
      assert(false);
  }
  ItePtr& bdd_graph = compute_table_[sig];  // Register if not computed.
  if (bdd_graph) return bdd_graph;  // Already computed.
  if (arg_one->order() == arg_two->order()) {  // The same variable.
    assert(arg_one->index() == arg_two->index());
    bdd_graph = std::make_shared<Ite>(arg_one->index(), arg_one->order());
    std::pair<bool, bool> term =
        bdd_graph->ApplyIfTerminal(type, arg_one, arg_two);
    if (!term.first)
      bdd_graph->high(Bdd::Apply(type, arg_one->high(), arg_two->high()));
    if (!term.second)
      bdd_graph->low(Bdd::Apply(type, arg_one->low(), arg_two->low()));
  } else {
    ItePtr v_one;  // Lower-order-vertex function graph.
    ItePtr v_two;  // Higher-order-vertex function graph.
    if (arg_one->order() < arg_two->order()) {
      v_one = arg_one;
      v_two = arg_two;
    } else {
      v_one = arg_two;
      v_two = arg_one;
    }
    bdd_graph = std::make_shared<Ite>(v_one->index(), v_one->order());
    std::pair<bool, bool> term = bdd_graph->ApplyIfTerminal(type, v_one, v_two);
    if (!term.first)
      bdd_graph->high(Bdd::Apply(type, v_one->high(), v_two));
    if (!term.second)
      bdd_graph->low(Bdd::Apply(type, v_one->low(), v_two));
  }
  ItePtr res = Bdd::ReduceGraph(bdd_graph);
  if (res) bdd_graph = res;
  return bdd_graph;
}

double Bdd::CalculateProbability(const ItePtr& vertex) noexcept {
  if (vertex->mark()) return vertex->prob();
  vertex->mark(true);
  double var_prob = probs_[vertex->index()];
  double high = vertex->high() ? Bdd::CalculateProbability(vertex->high())
                               : vertex->high_term()->value();
  double low = vertex->low() ? Bdd::CalculateProbability(vertex->low())
                               : vertex->low_term()->value();
  vertex->prob(var_prob * high  + (1 - var_prob) * low);
  return vertex->prob();
}

}  // namespace scram
