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
#include "logger.h"
#include "zbdd.h"

namespace scram {

Vertex::Vertex(bool terminal, int id) : id_(id), terminal_(terminal) {}

Vertex::~Vertex() {}  // Empty body for pure virtual destructor.

Terminal::Terminal(bool value) : Vertex::Vertex(true, value), value_(value) {}

NonTerminal::NonTerminal(int index, int order)
      : index_(index),
        order_(order),
        module_(false),
        mark_(false) {}

NonTerminal::~NonTerminal() {}  // Default pure virtual destructor.

ComplementEdge::ComplementEdge() : complement_edge_(false) {}

ComplementEdge::~ComplementEdge() {}  // Default pure virtual destructor.

Bdd::Bdd(const BooleanGraph* fault_tree, const Settings& settings)
    : kSettings_(settings),
      kOne_(std::make_shared<Terminal>(true)),
      function_id_(2) {
  CLOCK(init_time);
  LOG(DEBUG3) << "Converting Boolean graph into BDD...";
  if (fault_tree->root()->IsConstant()) {
    // Constant case should only happen to the top gate.
    if (fault_tree->root()->state() == kNullState) {
      root_ = {true, kOne_};
    } else {
      root_ = {false, kOne_};
    }
  } else {
    root_ = Bdd::IfThenElse(fault_tree->root());
  }
  LOG(DEBUG4) << "# of BDD vertices created: " << function_id_ - 1;
  LOG(DEBUG4) << "# of entries in unique table: " << unique_table_.size();
  LOG(DEBUG4) << "# of entries in compute table: " << compute_table_.size();
  Bdd::ClearMarks(false);
  LOG(DEBUG4) << "# of ITE in BDD: " << Bdd::CountIteNodes(root_.vertex);
  LOG(DEBUG3) << "Finished Boolean graph conversion in " << DUR(init_time);
  Bdd::ClearMarks(false);
}

Bdd::~Bdd() noexcept = default;

void Bdd::Analyze() noexcept {
  zbdd_ = std::unique_ptr<Zbdd>(new Zbdd(this, kSettings_));
  zbdd_->Analyze();
}

const std::vector<std::vector<int>>& Bdd::cut_sets() const {
  assert(zbdd_ && "Analysis is not done.");
  return zbdd_->cut_sets();
}

const Bdd::Function& Bdd::IfThenElse(const IGatePtr& gate) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  Function& result = gates_[gate->index()];
  if (result.vertex) return result;
  std::vector<Function> args;
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    args.push_back({arg.first < 0, Bdd::IfThenElse(arg.second)});
  }
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    const Function& res = Bdd::IfThenElse(arg.second);
    if (arg.second->IsModule()) {
      ItePtr proxy = Bdd::CreateModuleProxy(arg.second);
      args.push_back({arg.first < 0, proxy});
    } else {
      bool complement = (arg.first < 0) ^ res.complement;
      args.push_back({complement, res.vertex});
    }
  }
  std::sort(args.begin(), args.end(),
            [](const Function& lhs, const Function& rhs) {
    if (lhs.vertex->terminal()) return true;
    if (rhs.vertex->terminal()) return false;
    return Ite::Ptr(lhs.vertex)->order() > Ite::Ptr(rhs.vertex)->order();
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
  index_to_order_.emplace(variable->index(), variable->order());
  in_table = std::make_shared<Ite>(variable->index(), variable->order());
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
  in_table = std::make_shared<Ite>(gate->index(), gate->order());
  in_table->module(true);  // The main difference.
  in_table->id(function_id_++);
  in_table->high(kOne_);
  in_table->low(kOne_);
  in_table->complement_edge(true);
  return in_table;
}

Bdd::Function Bdd::Apply(Operator type,
                         const VertexPtr& arg_one, const VertexPtr& arg_two,
                         bool complement_one, bool complement_two) noexcept {
  assert(arg_one->id() && arg_two->id());  // Both are reduced function graphs.
  if (arg_one->terminal() && arg_two->terminal())
    return Bdd::Apply(type, Terminal::Ptr(arg_one), Terminal::Ptr(arg_two),
                      complement_one, complement_two);
  if (arg_one->terminal())
    return Bdd::Apply(type, Ite::Ptr(arg_two), Terminal::Ptr(arg_one),
                      complement_two, complement_one);
  if (arg_two->terminal())
    return Bdd::Apply(type, Ite::Ptr(arg_one), Terminal::Ptr(arg_two),
                      complement_one, complement_two);
  if (arg_one->id() == arg_two->id())  // Reduction detection.
    return Bdd::Apply(type, arg_one, complement_one, complement_two);

  Triplet sig =
      Bdd::GetSignature(type, arg_one, arg_two, complement_one, complement_two);
  Function& result = compute_table_[sig];  // Register if not computed.
  if (result.vertex) return result;  // Already computed.

  ItePtr ite_one = Ite::Ptr(arg_one);
  ItePtr ite_two = Ite::Ptr(arg_two);
  if (ite_one->order() > ite_two->order()) {
    std::swap(ite_one, ite_two);
    std::swap(complement_one, complement_two);
  }
  std::pair<Function, Function> new_edges = Bdd::Apply(type, ite_one, ite_two,
                                                       complement_one,
                                                       complement_two);
  Function& high = new_edges.first;
  Function& low = new_edges.second;
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

Bdd::Function Bdd::Apply(Operator type, const TerminalPtr& term_one,
                         const TerminalPtr& term_two, bool complement_one,
                         bool complement_two) noexcept {
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

Bdd::Function Bdd::Apply(Operator type,
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

Bdd::Function Bdd::Apply(Operator type, const VertexPtr& single_arg,
                         bool complement_one, bool complement_two) noexcept {
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
  return {complement_one, single_arg};
}

std::pair<Bdd::Function, Bdd::Function>
Bdd::Apply(Operator type, const ItePtr& arg_one, const ItePtr& arg_two,
           bool complement_one, bool complement_two) noexcept {
  if (arg_one->order() == arg_two->order()) {  // The same variable.
    assert(arg_one->index() == arg_two->index());
    Function high = Bdd::Apply(type, arg_one->high(), arg_two->high(),
                               complement_one, complement_two);
    Function low = Bdd::Apply(type, arg_one->low(), arg_two->low(),
                              complement_one ^ arg_one->complement_edge(),
                              complement_two ^ arg_two->complement_edge());
    return {high, low};
  }
  assert(arg_one->order() < arg_two->order());
  Function high = Bdd::Apply(type, arg_one->high(), arg_two,
                             complement_one, complement_two);
  Function low = Bdd::Apply(type, arg_one->low(), arg_two,
                            complement_one ^ arg_one->complement_edge(),
                            complement_two);
  return {high, low};
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

  std::array<int, 3> sig;  // Signature of the operation.
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

int Bdd::CountIteNodes(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 0;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark()) return 0;
  ite->mark(true);
  int in_module = 0;
  if (ite->module()) {
    const Function& module = gates_.find(ite->index())->second;
    in_module = Bdd::CountIteNodes(module.vertex);
  }
  return 1 + in_module + Bdd::CountIteNodes(ite->high()) +
         Bdd::CountIteNodes(ite->low());
}

void Bdd::ClearMarks(const VertexPtr& vertex, bool mark) noexcept {
  if (vertex->terminal()) return;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return;
  ite->mark(mark);
  if (ite->module()) {
    const Bdd::Function& res = gates_.find(ite->index())->second;
    Bdd::ClearMarks(res.vertex, mark);
  }
  Bdd::ClearMarks(ite->high(), mark);
  Bdd::ClearMarks(ite->low(), mark);
}

}  // namespace scram
