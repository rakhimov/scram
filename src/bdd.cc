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
#ifndef NGARBAGE
      garbage_collection_(std::make_shared<bool>(true)),
#endif
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
  } else if (fault_tree->root()->type() == kNullGate) {
    IGatePtr top = fault_tree->root();
    assert(top->args().size() == 1);
    assert(top->gate_args().empty());
    int child = *top->args().begin();
    VariablePtr var = top->variable_args().begin()->second;
    root_ = {child < 0, Bdd::FetchUniqueTable(var->index(), kOne_, kOne_,
                                              true, var->order(), false)};
  } else {
    std::unordered_map<int, Function> gates;
    root_ = Bdd::IfThenElse(fault_tree->root(), &gates);
  }
  Bdd::ClearMarks(false);
  Bdd::TestStructure(root_.vertex);
  LOG(DEBUG4) << "# of BDD vertices created: " << function_id_ - 1;
  LOG(DEBUG4) << "# of entries in unique table: " << unique_table_.size();
  LOG(DEBUG4) << "# of entries in AND table: " << and_table_.size();
  LOG(DEBUG4) << "# of entries in OR table: " << or_table_.size();
  Bdd::ClearMarks(false);
  LOG(DEBUG4) << "# of ITE in BDD: " << Bdd::CountIteNodes(root_.vertex);
  LOG(DEBUG3) << "Finished Boolean graph conversion in " << DUR(init_time);
  Bdd::ClearMarks(false);

  // Cleanup.
#ifndef NGARBAGE
  garbage_collection_ = nullptr;  // For faster cleanup.
  ite_as_arg_.clear();
  ite_as_arg_.reserve(0);
#endif
  LOG(DEBUG5) << "BDD switched off the garbage collector.";
  unique_table_.clear();
  unique_table_.reserve(0);
  and_table_.clear();
  and_table_.reserve(0);
  or_table_.clear();
  or_table_.reserve(0);
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

#ifndef NGARBAGE
void Bdd::GarbageCollector::operator()(Ite* ptr) noexcept {
  if (!garbage_collection_.expired()) {
    LOG(DEBUG5) << "Running garbage collection for " << ptr->id();
    bdd_->unique_table_.erase(
        {ptr->index(),
         ptr->high()->id(),
         (ptr->complement_edge() ? -1 : 1) * ptr->low()->id()});
    auto it = bdd_->ite_as_arg_.find(ptr->id());
    if (it != bdd_->ite_as_arg_.end()) {
      Membership& member_in = it->second;
      for (const std::pair<int, int>& key : member_in.and_table)
        bdd_->and_table_.erase(key);

      for (const std::pair<int, int>& key : member_in.or_table)
        bdd_->or_table_.erase(key);

      bdd_->ite_as_arg_.erase(it);
    }
  }
  delete ptr;
}
#endif

ItePtr Bdd::FetchUniqueTable(int index, const VertexPtr& high,
                             const VertexPtr& low, bool complement_edge,
                             int order, bool module) noexcept {
  assert(index > 0 && "Only positive indices are expected.");
  int sign = complement_edge ? -1 : 1;
  IteWeakPtr& in_table = unique_table_[{index, high->id(), sign * low->id()}];
  if (!in_table.expired()) return in_table.lock();
  assert(order > 0 && "Improper order.");
#ifndef NGARBAGE
  ItePtr ite(new Ite(index, order), GarbageCollector(this));
#else
  ItePtr ite(new Ite(index, order));
#endif
  ite->id(function_id_++);
  ite->module(module);
  ite->high(high);
  ite->low(low);
  ite->complement_edge(complement_edge);
  in_table = ite;
  return ite;
}

const Bdd::Function& Bdd::IfThenElse(
    const IGatePtr& gate,
    std::unordered_map<int, Function>* gates) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  Function& result = (*gates)[gate->index()];
  if (result.vertex) return result;
  std::vector<Function> args;
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    args.push_back({arg.first < 0,
                    Bdd::FetchUniqueTable(arg.second->index(), kOne_, kOne_,
                                          true, arg.second->order(), false)});
    index_to_order_.emplace(arg.second->index(), arg.second->order());
  }
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    const Function& res = Bdd::IfThenElse(arg.second, gates);
    if (arg.second->IsModule()) {
      args.push_back({arg.first < 0,
                      Bdd::FetchUniqueTable(arg.second->index(), kOne_, kOne_,
                                            true, arg.second->order(), true)});
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
  if (gate->IsModule()) modules_.emplace(gate->index(), result);
  return result;
}

Bdd::Function& Bdd::FetchComputeTable(Operator type,
                                      const VertexPtr& arg_one,
                                      const VertexPtr& arg_two,
                                      bool complement_one,
                                      bool complement_two) noexcept {
  assert(!arg_one->terminal() && !arg_two->terminal());
  assert(arg_one->id() && arg_two->id());
  assert(arg_one->id() != arg_two->id());
  int min_id = arg_one->id() * (complement_one ? -1 : 1);
  int max_id = arg_two->id() * (complement_two ? -1 : 1);
  if (arg_one->id() > arg_two->id()) std::swap(min_id, max_id);
#ifndef NGARBAGE
  Membership& member_one = ite_as_arg_[arg_one->id()];
  Membership& member_two = ite_as_arg_[arg_two->id()];
  switch (type) {  /// @todo Detect equal calculations with complements.
    case kOrGate:
      member_one.or_table.emplace_back(min_id, max_id);
      member_two.or_table.emplace_back(min_id, max_id);
      return or_table_[{min_id, max_id}];
    case kAndGate:
      member_one.and_table.emplace_back(min_id, max_id);
      member_two.and_table.emplace_back(min_id, max_id);
      return and_table_[{min_id, max_id}];
    default:
      assert(false);
  }
#else
  switch (type) {
    case kOrGate:
      return or_table_[{min_id, max_id}];
    case kAndGate:
      return and_table_[{min_id, max_id}];
    default:
      assert(false);
  }
#endif
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

  Function& result = Bdd::FetchComputeTable(type, arg_one, arg_two,
                                            complement_one, complement_two);
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
  result.complement = high.complement;
  bool complement_edge = high.complement ^ low.complement;
  if (!complement_edge && (high.vertex->id() == low.vertex->id())) {
      result.vertex = low.vertex;  // Another redundancy detection.
      return result;
  }
  result.vertex = Bdd::FetchUniqueTable(ite_one->index(), high.vertex,
                                        low.vertex, complement_edge,
                                        ite_one->order(), ite_one->module());
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

int Bdd::CountIteNodes(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 0;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark()) return 0;
  ite->mark(true);
  int in_module = 0;
  if (ite->module()) {
    const Function& module = modules_.find(ite->index())->second;
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
    const Bdd::Function& res = modules_.find(ite->index())->second;
    Bdd::ClearMarks(res.vertex, mark);
  }
  Bdd::ClearMarks(ite->high(), mark);
  Bdd::ClearMarks(ite->low(), mark);
}

void Bdd::TestStructure(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark()) return;
  ite->mark(true);
  assert(ite->index() && "Illegal index for a node.");
  assert(ite->order() && "Improper order for nodes.");
  assert(ite->high() && ite->low() && "Malformed node high/low pointers.");
  assert(
      !(!ite->complement_edge() && ite->high()->id() == ite->low()->id()) &&
      "Reduction rule failure.");
  assert(!(!ite->high()->terminal() &&
           ite->order() >= Ite::Ptr(ite->high())->order()) &&
         "Ordering of nodes failed.");
  assert(!(!ite->low()->terminal() &&
           ite->order() >= Ite::Ptr(ite->low())->order()) &&
         "Ordering of nodes failed.");
  if (ite->module()) {
    const Bdd::Function& res = modules_.find(ite->index())->second;
    assert(!res.vertex->terminal() && "Terminal modules must be removed.");
    Bdd::TestStructure(res.vertex);
  }
  Bdd::TestStructure(ite->high());
  Bdd::TestStructure(ite->low());
}

}  // namespace scram
