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
        module_(false),
        complement_edge_(false),
        mark_(false) {}

NonTerminal::~NonTerminal() {}  // Default pure virtual destructor.

Bdd::Bdd(BooleanGraph* fault_tree)
    : fault_tree_(fault_tree),
      complement_root_(false),
      p_graph_(0),
      kOne_(std::make_shared<Terminal>(true)),
      function_id_(2) {}

void Bdd::Analyze() noexcept {
  const Result& result = Bdd::IfThenElse(fault_tree_->root());
  root_ = result.vertex;
  complement_root_ = result.complement;

  probs_.reserve(fault_tree_->basic_events().size() + 1);
  probs_.push_back(-1);  // First one is a dummy.
  using BasicEventPtr = std::shared_ptr<BasicEvent>;
  for (const BasicEventPtr& event : fault_tree_->basic_events()) {
    probs_.push_back(event->p());
  }
  p_graph_ = Bdd::CalculateProbability(root_);
  if (complement_root_) p_graph_ = 1 - p_graph_;
}

const Bdd::Result& Bdd::IfThenElse(const IGatePtr& gate) noexcept {
  Result& result = gates_[gate->index()];
  if (result.vertex) return result;
  if (gate->state() != kNormalState) {
    // Constant case should only happen to the top gate.
    assert(gate == fault_tree_->root());
    if (gate->state() == kNullState) {
      result = {true, kOne_};
    } else {
      result = {false, kOne_};
    }
    return result;
  }
  std::vector<Result> args;
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    args.push_back({arg.first < 0, Bdd::IfThenElse(arg.second)});
  }
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    const Result& res = Bdd::IfThenElse(arg.second);
    if (arg.second->IsModule()) {
      ItePtr proxy = Bdd::CreateModuleProxy(arg.second);
      args.push_back({arg.first < 0, proxy});
    } else {
      bool complement = (arg.first < 0) ^ res.complement;
      args.push_back({complement, res.vertex});
    }
  }
  std::sort(args.begin(), args.end(),
            [](const Result& lhs, const Result& rhs) {
              if (lhs.vertex->terminal()) return false;
              if (rhs.vertex->terminal()) return true;
              return Ite::Ptr(lhs.vertex)->order() <
                     Ite::Ptr(rhs.vertex)->order();
            });
  auto it = args.cbegin();
  result.complement = it->complement;
  result.vertex = it->vertex;
  for (++it; it != args.cend(); ++it) {
    result = Bdd::Apply(gate->type(), result.vertex, it->vertex,
                        result.complement, it->complement);
  }
  assert(result.vertex);
  return result;
}

std::shared_ptr<Ite> Bdd::IfThenElse(const VariablePtr& variable) noexcept {
  ItePtr& in_table = unique_table_[{variable->index(), 1, -1}];
  if (in_table) return in_table;
  in_table = std::make_shared<Ite>(variable->index(), variable->opti_value());
  in_table->id(function_id_++);
  in_table->high(kOne_);
  in_table->low(kOne_);
  in_table->complement_edge(true);
  return in_table;
}

std::shared_ptr<Ite> Bdd::CreateModuleProxy(const IGatePtr& gate) noexcept {
  assert(gate->IsModule());
  ItePtr& in_table = unique_table_[{gate->index(), 1, -1}];
  if (in_table) return in_table;
  in_table = std::make_shared<Ite>(gate->index(), gate->opti_value());
  in_table->module(true);  // The main difference.
  in_table->id(function_id_++);
  in_table->high(kOne_);
  in_table->low(kOne_);
  in_table->complement_edge(true);
  return in_table;
}

std::shared_ptr<Vertex> Bdd::Reduce(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return vertex;
  if (vertex->id()) return vertex;  // Already reduced function graph.
  ItePtr ite = Ite::Ptr(vertex);
  ite->high(Bdd::Reduce(ite->high()));
  ite->low(Bdd::Reduce(ite->low()));
  if (ite->high()->id() == ite->low()->id()) {  // Redundancy condition.
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

Bdd::Result Bdd::Apply(Operator type,
                       const VertexPtr& arg_one, const VertexPtr& arg_two,
                       bool complement_one, bool complement_two) noexcept {
  if (arg_one->terminal() && arg_two->terminal()) {
    return Bdd::Apply(type, Terminal::Ptr(arg_one), Terminal::Ptr(arg_two),
                      complement_one, complement_two);
  } else if (arg_one->terminal()) {
    return Bdd::Apply(type, Ite::Ptr(arg_two), Terminal::Ptr(arg_one),
                      complement_two, complement_one);
  } else if (arg_two->terminal()) {
    return Bdd::Apply(type, Ite::Ptr(arg_one), Terminal::Ptr(arg_two),
                      complement_one, complement_two);
  }
  assert(arg_one->id() && arg_two->id());  // Both are reduced function graphs.
  if (arg_one->id() == arg_two->id()) {  // Reduction detection.
    if (complement_one ^ complement_two) {
      switch (type) {
        case kOrGate:
          return {false, kOne_};
        case kAndGate:
          return {true, kOne_};
        default:
          assert(false);
      }
    }
    return {complement_one, arg_one};
  }
  Triplet sig =
      Bdd::GetSignature(type, arg_one, arg_two, complement_one, complement_two);
  Result& result = compute_table_[sig];  // Register if not computed.
  if (result.vertex) return result;  // Already computed.

  ItePtr ite_one = Ite::Ptr(arg_one);
  ItePtr ite_two = Ite::Ptr(arg_two);
  Result high;
  Result low;
  if (ite_one->order() == ite_two->order()) {  // The same variable.
    assert(ite_one->index() == ite_two->index());
    high = Bdd::Apply(type, ite_one->high(), ite_two->high(),
                      complement_one, complement_two);
    low = Bdd::Apply(type, ite_one->low(), ite_two->low(),
                     complement_one ^ ite_one->complement_edge(),
                     complement_two ^ ite_two->complement_edge());
  } else {
    if (ite_one->order() > ite_two->order()) {
      std::swap(ite_one, ite_two);
      std::swap(complement_one, complement_two);
    }
    high = Bdd::Apply(type, ite_one->high(), ite_two,
                      complement_one, complement_two);
    low = Bdd::Apply(type, ite_one->low(), ite_two,
                     complement_one ^ ite_one->complement_edge(),
                     complement_two);
  }
  ItePtr bdd_graph = std::make_shared<Ite>(ite_one->index(), ite_one->order());
  bdd_graph->module(ite_one->module());  /// @todo Create clone function.
  bdd_graph->high(high.vertex);
  bdd_graph->low(low.vertex);
  if (high.complement) {
    result.complement = true;
    bdd_graph->complement_edge(!low.complement);
  } else {
    result.complement = false;
    bdd_graph->complement_edge(low.complement);
  }
  if (!bdd_graph->complement_edge()) {  // Another redundancy detection.
    if (bdd_graph->high()->id() == bdd_graph->low()->id()) {
      result.vertex = bdd_graph->low();
      return result;
    }
  }
  int low_sign = bdd_graph->complement_edge() ? -1 : 1;
  ItePtr& in_table = unique_table_[{bdd_graph->index(), bdd_graph->high()->id(),
                                    low_sign * bdd_graph->low()->id()}];
  if (!in_table) {
    in_table = bdd_graph;
    in_table->id(function_id_++);  // Unique function graph.
  }
  result.vertex = in_table;
  return result;
}

Bdd::Result Bdd::Apply(Operator type,
                       const TerminalPtr& term_one, const TerminalPtr& term_two,
                       bool complement_one, bool complement_two) noexcept {
  assert(term_one->value());
  assert(term_two->value());
  assert(term_one == term_two);
  switch (type) {
    case kOrGate:
      if (complement_one && complement_two) return {true, kOne_};
      return {false, kOne_};
    case kAndGate:  // Reverse of OR logic!
      if (complement_one || complement_two) return {true, kOne_};
      return {false, kOne_};
    default:
      assert(false);
  }
}

Bdd::Result Bdd::Apply(Operator type,
                       const ItePtr& ite_one, const TerminalPtr& term_two,
                       bool complement_one, bool complement_two) noexcept {
  assert(term_two->value());
  switch (type) {
    case kOrGate:
      if (complement_two) return {complement_one, ite_one};
      return {false, kOne_};
    case kAndGate:
      if (complement_two) return {true, kOne_};
      return {complement_one, ite_one};
    default:
      assert(false);
  }
}

Triplet Bdd::GetSignature(Operator type,
                          const VertexPtr& arg_one, const VertexPtr& arg_two,
                          bool complement_one, bool complement_two) noexcept {
  assert(!arg_one->terminal() && !arg_two->terminal());
  assert(arg_one->id() && arg_two->id());
  assert(arg_one->id() != arg_two->id());
  int sign_one = complement_one ? -1 : 1;
  int sign_two = complement_two ? -1 : 1;
  int min_id = std::min(arg_one->id(), arg_two->id());
  int max_id = std::max(arg_one->id(), arg_two->id());
  min_id *= min_id == arg_one->id() ? sign_one : sign_two;
  max_id *= max_id == arg_one->id() ? sign_one : sign_two;

  std::array<int, 3> sig = {0, 0, 0};  // Signature of the operation.
  switch (type) {  /// @todo Detect equal calculations with complements.
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
  return sig;
}

double Bdd::CalculateProbability(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 1;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark()) return ite->prob();
  ite->mark(true);
  double var_prob = 0;
  if (ite->module()) {
    const Result& res = gates_[ite->index()];
    var_prob = Bdd::CalculateProbability(res.vertex);
    if (res.complement) var_prob = 1 - var_prob;
  } else {
    var_prob = probs_[ite->index()];
  }
  double high = Bdd::CalculateProbability(ite->high());
  double low = Bdd::CalculateProbability(ite->low());
  if (ite->complement_edge()) low = 1 - low;
  ite->prob(var_prob * high  + (1 - var_prob) * low);
  return ite->prob();
}

}  // namespace scram
