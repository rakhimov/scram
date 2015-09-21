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

Vertex::Vertex(bool terminal, int id) : id_(id), terminal_(terminal) {}

Vertex::~Vertex() {}  // Empty body for pure virtual destructor.

Terminal::Terminal(bool value) : Vertex::Vertex(true, value), value_(value) {}

NonTerminal::NonTerminal(int index, int order)
      : index_(index),
        order_(order),
        mark_(false) {}

NonTerminal::~NonTerminal() {}  // Default pure virtual destructor.

Bdd::Bdd(BooleanGraph* fault_tree)
    : fault_tree_(fault_tree),
      p_graph_(0),
      kOne_(std::make_shared<Terminal>(true)),
      kZero_(std::make_shared<Terminal>(false)),
      function_id_(2) {}

void Bdd::Analyze() noexcept {
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
    bdd_graph = Ite::Ptr(Bdd::Reduce(bdd_graph));
  }

  // Apply the gate operator to BDD graphs of argument variables and gates.
  /// @todo Determine if order matters among gates.
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    ItePtr arg_graph = Bdd::ConvertGate(arg.second);
    if (!bdd_graph) {  // No variable arguments have been processed.
      bdd_graph = arg_graph;
      continue;
    }
    VertexPtr result = Bdd::Apply(gate->type(), bdd_graph, arg_graph);
    bdd_graph = Ite::Ptr(result);  /// @todo Consider when this is not the case.
  }
  assert(bdd_graph);
  return bdd_graph;
}

std::shared_ptr<Vertex> Bdd::Reduce(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return vertex;
  if (vertex->id()) return vertex;  // Already reduced function graph.
  ItePtr ite = Ite::Ptr(vertex);
  ite->high(Bdd::Reduce(ite->high()));
  ite->low(Bdd::Reduce(ite->low()));
  if (ite->high()->id() == ite->low()->id()) {  // Redundancy condition.
    assert(!ite->low()->terminal());  /// @todo Generalize this.
    assert(ite->low() == ite->high());
    return ite->low();
  }
  ItePtr& in_table =
      unique_table_[{ite->index(), ite->high()->id(), ite->low()->id()}];
  if (in_table) return in_table;  // Existing function graph.
  ite->id(function_id_++);  // Unique function graph.
  in_table = ite;
  return ite;
}

std::shared_ptr<Ite> Bdd::IfThenElse(
    Operator type,
    const std::vector<std::pair<int, NodePtr>>& args,
    int pos) noexcept {
  assert(pos >= 0);
  assert(pos < args.size());
  const auto& arg = args[pos];
  ++pos;  // Next argument for subgraphs.
  ItePtr ite(new Ite(arg.second->index(), arg.second->opti_value()));
  VertexPtr high;
  VertexPtr low;
  switch (type) {
    case kOrGate:
      high = kOne_;
      if (pos == args.size()) {
        low = kZero_;
      } else {
        low = Bdd::IfThenElse(type, args, pos);
      }
      break;
    case kAndGate:
      if (pos == args.size()) {
        high = kOne_;
      } else {
        high = Bdd::IfThenElse(type, args, pos);
      }
      low = kZero_;
      break;
    default:
      assert(false);
  }
  if (arg.first > 0) {
    ite->high(high);
    ite->low(low);
  } else {
    ite->high(low);
    ite->low(high);
  }
  return ite;
}

std::shared_ptr<Vertex> Bdd::Apply(Operator type, const VertexPtr& arg_one,
                                   const VertexPtr& arg_two) noexcept {
  if (arg_one->terminal() && arg_two->terminal()) {
    return Bdd::ApplyTerminal(type, Terminal::Ptr(arg_one),
                              Terminal::Ptr(arg_two));
  } else if (arg_one->terminal()) {
    return Bdd::ApplyTerminal(type, Ite::Ptr(arg_two), Terminal::Ptr(arg_one));
  } else if (arg_two->terminal()) {
    return Bdd::ApplyTerminal(type, Ite::Ptr(arg_one), Terminal::Ptr(arg_two));
  }
  std::array<int, 3> sig = {0, 0, 0};  // Signature of the operation.
  int min_id = std::min(arg_one->id(), arg_two->id());
  int max_id = std::max(arg_one->id(), arg_two->id());
  assert(min_id && max_id);  // Both are reduced function graphs.
  if (min_id == max_id) return arg_one;  // Reduction rule.
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

  ItePtr ite_one = Ite::Ptr(arg_one);
  ItePtr ite_two = Ite::Ptr(arg_two);

  if (ite_one->order() == ite_two->order()) {  // The same variable.
    assert(ite_one->index() == ite_two->index());
    bdd_graph = std::make_shared<Ite>(ite_one->index(), ite_one->order());
    bdd_graph->high(Bdd::Apply(type, ite_one->high(), ite_two->high()));
    bdd_graph->low(Bdd::Apply(type, ite_one->low(), ite_two->low()));
  } else {
    ItePtr v_one;  // Lower-order-vertex function graph.
    ItePtr v_two;  // Higher-order-vertex function graph.
    if (ite_one->order() < ite_two->order()) {
      v_one = ite_one;
      v_two = ite_two;
    } else {
      v_one = ite_two;
      v_two = ite_one;
    }
    bdd_graph = std::make_shared<Ite>(v_one->index(), v_one->order());
    bdd_graph->high(Bdd::Apply(type, v_one->high(), v_two));
    bdd_graph->low(Bdd::Apply(type, v_one->low(), v_two));
  }
  return Bdd::Reduce(bdd_graph);
}

std::shared_ptr<Terminal> Bdd::ApplyTerminal(
    Operator type,
    const TerminalPtr& term_one,
    const TerminalPtr& term_two) noexcept {
  switch (type) {
    case kOrGate:
      return term_one->value() ? term_one : term_two;
    case kAndGate:
      return term_one->value() ? term_two : term_one;  // Reverse of OR logic!
    default:
      assert(false);
  }
}

std::shared_ptr<Vertex> Bdd::ApplyTerminal(
    Operator type,
    const ItePtr& v_one,
    const TerminalPtr& term_one) noexcept {
  switch (type) {
    case kOrGate:
      if (term_one->value()) return term_one;
      return v_one;
    case kAndGate:
      if (term_one->value()) return v_one;
      return term_one;
    default:
      assert(false);
  }
}

double Bdd::CalculateProbability(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return Terminal::Ptr(vertex)->value();
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark()) return ite->prob();
  ite->mark(true);

  double var_prob = probs_[ite->index()];
  double high = Bdd::CalculateProbability(ite->high());
  double low = Bdd::CalculateProbability(ite->low());
  ite->prob(var_prob * high  + (1 - var_prob) * low);
  return ite->prob();
}

}  // namespace scram
