/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
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

#include <boost/multiprecision/miller_rabin.hpp>
#include <boost/range/algorithm.hpp>

#include "event.h"
#include "ext.h"
#include "logger.h"
#include "zbdd.h"

namespace scram {
namespace core {

int GetPrimeNumber(int n) {
  assert(n > 0 && "Only natural numbers.");
  if (n % 2 == 0) ++n;
  while (boost::multiprecision::miller_rabin_test(n, 25) == false) n += 2;
  return n;
}

Bdd::Bdd(const BooleanGraph* fault_tree, const Settings& settings)
    : kSettings_(settings),
      coherent_(fault_tree->coherent()),
      kOne_(new Terminal<Ite>(true)),
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
  } else if (fault_tree->root()->type() == kNull) {
    GatePtr top = fault_tree->root();
    assert(top->args().size() == 1);
    assert(top->args<Gate>().empty());
    int child = *top->args().begin();
    VariablePtr var = top->args<Variable>().begin()->second;
    root_ = {child < 0, Bdd::FindOrAddVertex(var->index(), kOne_, kOne_, true,
                                             var->order())};
  } else {
    std::unordered_map<int, std::pair<Function, int>> gates;
    root_ = Bdd::ConvertGraph(fault_tree->root(), &gates);
    root_.complement ^= fault_tree->complement();
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
  // Clear tables if no more calculations are expected.
  Bdd::ClearTables();
  if (coherent_) {
    unique_table_.Release();
    and_table_.reserve(0);
    or_table_.reserve(0);
  }
}

Bdd::~Bdd() noexcept = default;

void Bdd::Analyze() noexcept {
  zbdd_ = ext::make_unique<Zbdd>(this, kSettings_);
  zbdd_->Analyze();
  if (!coherent_) {  // The BDD has been used by the ZBDD.
    Bdd::ClearTables();
    unique_table_.Release();
    and_table_.reserve(0);
    or_table_.reserve(0);
  }
}

const std::vector<std::vector<int>>& Bdd::products() const {
  assert(zbdd_ && "Analysis is not done.");
  return zbdd_->products();
}

ItePtr Bdd::FindOrAddVertex(int index, const VertexPtr& high,
                            const VertexPtr& low, bool complement_edge,
                            int order) noexcept {
  assert(index > 0 && "Only positive indices are expected.");
  IteWeakPtr& in_table =
      unique_table_.FindOrAdd(index, high->id(),
                              complement_edge ? -low->id() : low->id());
  if (!in_table.expired()) return in_table.lock();
  assert(order > 0 && "Improper order.");
  ItePtr ite(new Ite(index, order, function_id_++, high, low));
  ite->complement_edge(complement_edge);
  in_table = ite;
  return ite;
}

ItePtr Bdd::FindOrAddVertex(const ItePtr& ite, const VertexPtr& high,
                            const VertexPtr& low,
                            bool complement_edge) noexcept {
  ItePtr in_table = Bdd::FindOrAddVertex(ite->index(), high, low,
                                         complement_edge, ite->order());
  if (in_table->unique()) {
    in_table->module(ite->module());
    in_table->coherent(ite->coherent());
  }
  assert(in_table->module() == ite->module());
  assert(in_table->coherent() == ite->coherent());
  return in_table;
}

ItePtr Bdd::FindOrAddVertex(const GatePtr& gate, const VertexPtr& high,
                            const VertexPtr& low,
                            bool complement_edge) noexcept {
  assert(gate->module() && "Only module gates are expected for proxies.");
  ItePtr in_table = Bdd::FindOrAddVertex(gate->index(), high, low,
                                         complement_edge, gate->order());
  if (in_table->unique()) {
    in_table->module(gate->module());
    in_table->coherent(gate->coherent());
  }
  assert(in_table->module() == gate->module());
  assert(in_table->coherent() == gate->coherent());
  return in_table;
}

Bdd::Function Bdd::ConvertGraph(
    const GatePtr& gate,
    std::unordered_map<int, std::pair<Function, int>>* gates) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  Function result;  // For the NRVO, due to memoization.
  // Memoization check.
  if (auto it_entry = ext::find(*gates, gate->index())) {
    std::pair<Function, int>& entry = it_entry->second;
    result = entry.first;
    assert(entry.second < gate->parents().size());  // Processed parents.
    if (++entry.second == gate->parents().size()) gates->erase(it_entry);
    return result;
  }
  std::vector<Function> args;
  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    args.push_back({arg.first < 0,
                    Bdd::FindOrAddVertex(arg.second->index(), kOne_, kOne_,
                                         true, arg.second->order())});
    index_to_order_.emplace(arg.second->index(), arg.second->order());
  }
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    Function res = Bdd::ConvertGraph(arg.second, gates);
    if (arg.second->module()) {
      args.push_back({arg.first < 0,
                      Bdd::FindOrAddVertex(arg.second, kOne_, kOne_, true)});
    } else {
      bool complement = (arg.first < 0) ^ res.complement;
      args.push_back({complement, res.vertex});
    }
  }
  boost::sort(args, [](const Function& lhs, const Function& rhs) {
    if (lhs.vertex->terminal()) return true;
    if (rhs.vertex->terminal()) return false;
    return Ite::Ptr(lhs.vertex)->order() > Ite::Ptr(rhs.vertex)->order();
  });
  auto it = args.cbegin();
  for (result = *it++; it != args.cend(); ++it) {
    result = Bdd::Apply(gate->type(), result.vertex, it->vertex,
                        result.complement, it->complement);
  }
  Bdd::ClearTables();
  assert(result.vertex);
  if (gate->module()) modules_.emplace(gate->index(), result);
  if (gate->parents().size() > 1) gates->insert({gate->index(), {result, 1}});
  return result;
}

std::pair<int, int> Bdd::GetMinMaxId(const VertexPtr& arg_one,
                                     const VertexPtr& arg_two,
                                     bool complement_one,
                                     bool complement_two) noexcept {
  assert(!arg_one->terminal() && !arg_two->terminal());
  assert(arg_one->id() && arg_two->id());
  assert(arg_one->id() != arg_two->id());
  int min_id = arg_one->id() * (complement_one ? -1 : 1);
  int max_id = arg_two->id() * (complement_two ? -1 : 1);
  if (arg_one->id() > arg_two->id()) std::swap(min_id, max_id);
  return {min_id, max_id};
}

/// Specialization of Apply for AND operator with BDD vertices.
template <>
Bdd::Function Bdd::Apply<kAnd>(const VertexPtr& arg_one,
                               const VertexPtr& arg_two, bool complement_one,
                               bool complement_two) noexcept {
  assert(arg_one->id() && arg_two->id());  // Both are reduced function graphs.
  if (arg_one->terminal()) {
    if (complement_one) return {true, kOne_};
    return {complement_two, arg_two};
  }
  if (arg_two->terminal()) {
    if (complement_two) return {true, kOne_};
    return {complement_one, arg_one};
  }
  if (arg_one->id() == arg_two->id()) {  // Reduction detection.
    if (complement_one ^ complement_two) return {true, kOne_};
    return {complement_one, arg_one};
  }
  std::pair<int, int> min_max_id =
      Bdd::GetMinMaxId(arg_one, arg_two, complement_one, complement_two);
  if (auto it = ext::find(and_table_, min_max_id)) return it->second;
  Function result = Bdd::Apply<kAnd>(Ite::Ptr(arg_one), Ite::Ptr(arg_two),
                                     complement_one, complement_two);
  and_table_.emplace(min_max_id, result);
  return result;
}

/// Specialization of Apply for OR operator with BDD vertices.
template <>
Bdd::Function Bdd::Apply<kOr>(const VertexPtr& arg_one,
                              const VertexPtr& arg_two, bool complement_one,
                              bool complement_two) noexcept {
  assert(arg_one->id() && arg_two->id());  // Both are reduced function graphs.
  if (arg_one->terminal()) {
    if (!complement_one) return {false, kOne_};
    return {complement_two, arg_two};
  }
  if (arg_two->terminal()) {
    if (!complement_two) return {false, kOne_};
    return {complement_one, arg_one};
  }
  if (arg_one->id() == arg_two->id()) {  // Reduction detection.
    if (complement_one ^ complement_two) return {false, kOne_};
    return {complement_one, arg_one};
  }
  std::pair<int, int> min_max_id =
      Bdd::GetMinMaxId(arg_one, arg_two, complement_one, complement_two);
  if (auto it = ext::find(or_table_, min_max_id)) return it->second;
  Function result = Bdd::Apply<kOr>(Ite::Ptr(arg_one), Ite::Ptr(arg_two),
                                    complement_one, complement_two);
  or_table_.emplace(min_max_id, result);
  return result;
}

template <Operator Type>
Bdd::Function Bdd::Apply(ItePtr ite_one, ItePtr ite_two,
                         bool complement_one,
                         bool complement_two) noexcept {
  if (ite_one->order() > ite_two->order()) {
    ite_one.swap(ite_two);
    std::swap(complement_one, complement_two);
  }

  Function high;
  Function low;
  if (ite_one->order() == ite_two->order()) {  // The same variable.
    assert(ite_one->index() == ite_two->index());
    high = Bdd::Apply<Type>(ite_one->high(), ite_two->high(), complement_one,
                            complement_two);
    low = Bdd::Apply<Type>(ite_one->low(), ite_two->low(),
                           complement_one ^ ite_one->complement_edge(),
                           complement_two ^ ite_two->complement_edge());
  } else {
    assert(ite_one->order() < ite_two->order());
    high = Bdd::Apply<Type>(ite_one->high(), ite_two, complement_one,
                            complement_two);
    low = Bdd::Apply<Type>(ite_one->low(), ite_two,
                           complement_one ^ ite_one->complement_edge(),
                           complement_two);
  }

  bool complement_edge = high.complement ^ low.complement;
  if (complement_edge || (high.vertex->id() != low.vertex->id())) {
    high.vertex =
        Bdd::FindOrAddVertex(ite_one, high.vertex, low.vertex, complement_edge);
  }

  return high;
}

Bdd::Function Bdd::Apply(Operator type,
                         const VertexPtr& arg_one, const VertexPtr& arg_two,
                         bool complement_one, bool complement_two) noexcept {
  assert(arg_one->id() && arg_two->id());  // Both are reduced function graphs.
  if (type == kAnd) {
    return Bdd::Apply<kAnd>(arg_one, arg_two, complement_one, complement_two);
  }
  assert(type == kOr && "Unsupported operator.");
  return Bdd::Apply<kOr>(arg_one, arg_two, complement_one, complement_two);
}

Bdd::Function Bdd::CalculateConsensus(const ItePtr& ite,
                                      bool complement) noexcept {
  Bdd::ClearTables();
  return Bdd::Apply<kAnd>(ite->high(), ite->low(), complement,
                          ite->complement_edge() ^ complement);
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

}  // namespace core
}  // namespace scram
