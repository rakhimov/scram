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

/// @file zbdd.cc
/// Implementation of Zero-Suppressed BDD algorithms.

#include "zbdd.h"

#include <algorithm>

#include "logger.h"

namespace scram {

Zbdd::Zbdd(const Settings& settings) noexcept
    : kSettings_(settings),
      unique_table_(std::make_shared<UniqueTable>()),
      kBase_(std::make_shared<Terminal>(true)),
      kEmpty_(std::make_shared<Terminal>(false)),
      set_id_(2) {}

/// @def LOG_ZBDD
/// Logs ZBDD characteristics.
#define LOG_ZBDD                                                               \
  LOG(DEBUG4) << "# of ZBDD nodes created: " << set_id_ - 1;                   \
  LOG(DEBUG4) << "# of entries in unique table: " << unique_table_->size();    \
  LOG(DEBUG4) << "# of entries in AND table: " << and_table_.size();           \
  LOG(DEBUG4) << "# of entries in OR table: " << or_table_.size();             \
  LOG(DEBUG4) << "# of entries in subsume table: " << subsume_table_.size();   \
  LOG(DEBUG4) << "# of entries in minimal table: " << minimal_results_.size(); \
  Zbdd::ClearMarks(root_);                                                     \
  LOG(DEBUG4) << "# of SetNodes in ZBDD: " << Zbdd::CountSetNodes(root_);      \
  Zbdd::ClearMarks(root_);                                                     \
  LOG(DEBUG3) << "There are " << Zbdd::CountProducts(root_) << " products.";   \
  Zbdd::ClearMarks(root_)

Zbdd::Zbdd(Bdd* bdd, const Settings& settings) noexcept
    : Zbdd::Zbdd(settings) {
  CLOCK(init_time);
  LOG(DEBUG2) << "Creating ZBDD from BDD...";
  const Bdd::Function& bdd_root = bdd->root();
  PairTable<VertexPtr> ites;
  root_ = Zbdd::ConvertBdd(bdd_root.vertex, bdd_root.complement, bdd,
                           kSettings_.limit_order(), &ites);
  Zbdd::ClearMarks(root_);
  Zbdd::TestStructure(root_);
  LOG_ZBDD;
  LOG(DEBUG2) << "Created ZBDD from BDD in " << DUR(init_time);
}

Zbdd::Zbdd(const BooleanGraph* fault_tree, const Settings& settings) noexcept
    : Zbdd::Zbdd(settings) {
  CLOCK(init_time);
  LOG(DEBUG2) << "Creating ZBDD from Boolean Graph...";
  if (fault_tree->root()->IsConstant()) {
    if (fault_tree->root()->state() == kNullState) {
      root_ = kEmpty_;
    } else {
      root_ = kBase_;
    }
  } else if (fault_tree->root()->type() == kNullGate) {
    IGatePtr top = fault_tree->root();
    assert(top->args().size() == 1);
    assert(top->gate_args().empty());
    int child = *top->args().begin();
    if (child < 0) {
      root_ = kBase_;
    } else {
      VariablePtr var = top->variable_args().begin()->second;
      root_ = Zbdd::FetchUniqueTable(var->index(), kBase_, kEmpty_,
                                     var->order(), false);
    }
  } else {
    std::unordered_map<int, std::pair<VertexPtr, int>> gates;
    root_ = Zbdd::ConvertGraph(fault_tree->root(), &gates);
    if (!fault_tree->coherent()) {
      Zbdd::ClearMarks(root_);
      Zbdd::TestStructure(root_);
      Zbdd::ClearMarks(root_);
      LOG(DEBUG5) << "Eliminating complements from ZBDD...";
      std::unordered_map<int, VertexPtr> results;
      root_ = Zbdd::EliminateComplements(root_, &results);
      std::vector<int> sub_modules;
      Zbdd::GatherModules(root_, &sub_modules);
      for (int i = 0; i < sub_modules.size(); ++i) {
        results.clear();
        VertexPtr& module = modules_.find(sub_modules[i])->second;
        module = Zbdd::EliminateComplements(module, &results);
        Zbdd::GatherModules(module, &sub_modules);
      }
      for (auto it = sub_modules.rbegin(); it != sub_modules.rend(); ++it) {
        results.clear();
        VertexPtr& module = modules_.find(*it)->second;
        module = Zbdd::EliminateConstantModules(module, &results);
      }
      results.clear();
      root_ = Zbdd::EliminateConstantModules(root_, &results);
      LOG(DEBUG5) << "Finished complement elimination.";
    }
  }
  Zbdd::ClearMarks(root_);
  Zbdd::TestStructure(root_);
  LOG_ZBDD;
  LOG(DEBUG2) << "Created ZBDD from Boolean Graph in " << DUR(init_time);
}

void Zbdd::Analyze() noexcept {
  CLOCK(analysis_time);
  LOG(DEBUG2) << "Analyzing ZBDD...";

  CLOCK(minimize_time);
  LOG(DEBUG3) << "Minimizing ZBDD...";
  for (std::pair<const int, VertexPtr>& module : modules_)
    module.second = Zbdd::Minimize(module.second);
  root_ = Zbdd::Minimize(root_);
  assert(root_->terminal() || SetNode::Ptr(root_)->minimal());
  Zbdd::ClearMarks(root_);
  Zbdd::TestStructure(root_);
  LOG_ZBDD;
  LOG(DEBUG3) << "Finished ZBDD minimization in " << DUR(minimize_time);

  // Complete cleanup of the memory.
  minimal_results_.clear();
  unique_table_.reset();  // Important to turn the garbage collector off.
  and_table_.clear();
  or_table_.clear();
  subsume_table_.clear();

  CLOCK(gen_time);
  LOG(DEBUG3) << "Getting products from minimized ZBDD...";
  products_ = Zbdd::GenerateProducts(root_);

  // Cleanup of temporary products.
  modules_.clear();
  root_ = kBase_;

  LOG(DEBUG3) << products_.size() << " products are found in " << DUR(gen_time);
  LOG(DEBUG2) << "Finished ZBDD analysis in " << DUR(analysis_time);
}

#undef LOG_ZBDD

void Zbdd::GarbageCollector::operator()(SetNode* ptr) noexcept {
  if (!unique_table_.expired()) {
    LOG(DEBUG5) << "Running garbage collection for " << ptr->id();
    unique_table_.lock()->erase({ptr->index(), ptr->high()->id(),
                                 ptr->low()->id()});
  }
  delete ptr;
}

SetNodePtr Zbdd::FetchUniqueTable(int index, const VertexPtr& high,
                                  const VertexPtr& low, int order,
                                  bool module) noexcept {
  SetNodeWeakPtr& in_table = (*unique_table_)[{index, high->id(), low->id()}];
  if (!in_table.expired()) return in_table.lock();
  assert(order > 0 && "Improper order.");
  SetNodePtr node(new SetNode(index, order, set_id_++, high, low),
                  GarbageCollector(this));
  node->module(module);
  in_table = node;
  return node;
}

SetNodePtr Zbdd::FetchUniqueTable(const SetNodePtr& node, const VertexPtr& high,
                                  const VertexPtr& low) noexcept {
  if (node->high()->id() == high->id() &&
      node->low()->id() == low->id()) return node;
  return Zbdd::FetchUniqueTable(node->index(), high, low, node->order(),
                                node->module());
}

VertexPtr Zbdd::FetchReducedVertex(int index, const VertexPtr& high,
                                   const VertexPtr& low, int order,
                                   bool module) noexcept {
  if (high->id() == low->id()) return low;
  if (high->terminal() && !Terminal::Ptr(high)->value()) return low;
  return Zbdd::FetchUniqueTable(index, high, low, order, module);
}

VertexPtr Zbdd::ConvertBdd(const VertexPtr& vertex, bool complement,
                           Bdd* bdd_graph, int limit_order,
                           PairTable<VertexPtr>* ites) noexcept {
  if (vertex->terminal()) return complement ? kEmpty_ : kBase_;
  int sign = complement ? -1 : 1;
  VertexPtr& result = (*ites)[{sign * vertex->id(), limit_order}];
  if (result) return result;
  if (kSettings_.prime_implicants()) {
    result = Zbdd::ConvertBddPI(Ite::Ptr(vertex), complement, bdd_graph,
                                limit_order, ites);
  } else {
    result = Zbdd::ConvertBdd(Ite::Ptr(vertex), complement, bdd_graph,
                              limit_order, ites);
  }
  return result;
}

VertexPtr Zbdd::ConvertBdd(const ItePtr& ite, bool complement,
                           Bdd* bdd_graph, int limit_order,
                           PairTable<VertexPtr>* ites) noexcept {
  VertexPtr low =
      Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                       bdd_graph, limit_order, ites);
  if (limit_order == 0) {  // Cut-off on the set order.
    if (low->terminal()) return low;
    return kEmpty_;
  }
  if (ite->module()) {  // This is a proxy and not a variable.
    const Bdd::Function& module =
        bdd_graph->modules().find(ite->index())->second;
    assert(!module.vertex->terminal() && "Unexpected BDD terminal module.");
    VertexPtr module_set =
        Zbdd::ConvertBdd(module.vertex, module.complement,
                         bdd_graph, kSettings_.limit_order(), ites);
    modules_.emplace(ite->index(), module_set);
    if (module_set->terminal()) {
      if (!Terminal::Ptr(module_set)->value()) return low;
      VertexPtr high = Zbdd::ConvertBdd(ite->high(), complement, bdd_graph,
                                        limit_order, ites);
      return Zbdd::Apply(kOrGate, high, low, kSettings_.limit_order());
    }
  }
  VertexPtr high =
      Zbdd::ConvertBdd(ite->high(), complement, bdd_graph, --limit_order, ites);
  if ((high->terminal() && !Terminal::Ptr(high)->value()) ||
      (high->id() == low->id()) ||
      (low->terminal() && Terminal::Ptr(low)->value())) {
    return low;  // Reduce and minimize.
  }
  return Zbdd::FetchUniqueTable(ite->index(), high, low, ite->order(),
                                ite->module());
}

VertexPtr Zbdd::ConvertBddPI(const ItePtr& ite, bool complement,
                             Bdd* bdd_graph, int limit_order,
                             PairTable<VertexPtr>* ites) noexcept {
  Bdd::Function common = bdd_graph->CalculateConsensus(ite, complement);
  VertexPtr consensus = Zbdd::ConvertBdd(common.vertex, common.complement,
                                         bdd_graph, limit_order, ites);
  if (limit_order == 0) {  // Cut-off on the product order.
    if (consensus->terminal()) return consensus;
    return kEmpty_;
  }
  if (ite->module()) {  // This is a proxy and not a variable.
    const Bdd::Function& module =
        bdd_graph->modules().find(ite->index())->second;
    assert(!module.vertex->terminal() && "Unexpected BDD terminal module.");
    VertexPtr module_pos =
        Zbdd::ConvertBdd(module.vertex, module.complement,
                         bdd_graph, kSettings_.limit_order(), ites);
    modules_.emplace(ite->index(), module_pos);
    assert(!module_pos->terminal());
    VertexPtr module_neg =
        Zbdd::ConvertBdd(module.vertex, !module.complement,
                         bdd_graph, kSettings_.limit_order(), ites);
    modules_.emplace(-ite->index(), module_neg);
    assert(!module_neg->terminal());
  }
  int sublimit = limit_order - 1;
  VertexPtr high =
      Zbdd::ConvertBdd(ite->high(), complement, bdd_graph, sublimit, ites);
  VertexPtr low =
      Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                       bdd_graph, sublimit, ites);
  return Zbdd::FetchReducedVertex(
      ite->index(), high,
      Zbdd::FetchReducedVertex(-ite->index(), low, consensus, ite->order(),
                               ite->module()),
      ite->order(), ite->module());
}

VertexPtr Zbdd::ConvertGraph(
    const IGatePtr& gate,
    std::unordered_map<int, std::pair<VertexPtr, int>>* gates) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  VertexPtr result;
  if (gates->count(gate->index())) {
    std::pair<VertexPtr, int>& entry = gates->find(gate->index())->second;
    result = entry.first;
    assert(entry.second < gate->parents().size());
    entry.second++;
    if (entry.second == gate->parents().size()) gates->erase(gate->index());
    return result;
  }
  std::vector<VertexPtr> args;
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    args.push_back(Zbdd::FetchUniqueTable(arg.first, kBase_, kEmpty_,
                                          arg.second->order(), false));
  }
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    assert(arg.first > 0 && "Complements must be pushed down to variables.");
    VertexPtr res = Zbdd::ConvertGraph(arg.second, gates);
    if (arg.second->IsModule()) {
      if (res->terminal()) {
        args.push_back(res);
      } else {
        args.push_back(Zbdd::FetchUniqueTable(arg.first, kBase_, kEmpty_,
                                              arg.second->order(), true));
      }
    } else {
      args.push_back(res);
    }
  }
  std::sort(args.begin(), args.end(),
            [](const VertexPtr& lhs, const VertexPtr& rhs) {
    if (lhs->terminal()) return true;
    if (rhs->terminal()) return false;
    return SetNode::Ptr(lhs)->order() > SetNode::Ptr(rhs)->order();
  });
  auto it = args.cbegin();
  result = *it;
  for (++it; it != args.cend(); ++it) {
    result = Zbdd::Apply(gate->type(), result, *it, kSettings_.limit_order());
  }
  and_table_.clear();
  or_table_.clear();
  subsume_table_.clear();
  minimal_results_.clear();
  assert(result);
  if (gate->IsModule()) modules_.emplace(gate->index(), result);
  if (gate->parents().size() > 1) gates->insert({gate->index(), {result, 1}});
  return result;
}

VertexPtr& Zbdd::FetchComputeTable(Operator type, const VertexPtr& arg_one,
                                   const VertexPtr& arg_two,
                                   int order) noexcept {
  assert(order >= 0 && "Illegal order for computations.");
  assert(!arg_one->terminal() && !arg_two->terminal());
  assert(arg_one->id() && arg_two->id());
  assert(arg_one->id() != arg_two->id());
  int min_id = std::min(arg_one->id(), arg_two->id());
  int max_id = std::max(arg_one->id(), arg_two->id());
  switch (type) {
    case kOrGate:
      return or_table_[{min_id, max_id, order}];
    case kAndGate:
      return and_table_[{min_id, max_id, order}];
    default:
      assert(false && "Unsupported Boolean operation!");
  }
}

VertexPtr Zbdd::Apply(Operator type, const VertexPtr& arg_one,
                      const VertexPtr& arg_two, int limit_order) noexcept {
  if (limit_order < 0) return kEmpty_;
  if (arg_one->terminal())
    return Zbdd::Apply(type, Terminal::Ptr(arg_one), arg_two);
  if (arg_two->terminal())
    return Zbdd::Apply(type, Terminal::Ptr(arg_two), arg_one);

  if (arg_one->id() == arg_two->id()) return arg_one;

  VertexPtr& result = Zbdd::FetchComputeTable(type, arg_one, arg_two,
                                              limit_order);
  if (result) return result;  // Already computed.

  SetNodePtr set_one = SetNode::Ptr(arg_one);
  SetNodePtr set_two = SetNode::Ptr(arg_two);
  if (set_one->order() > set_two->order()) std::swap(set_one, set_two);
  if (set_one->order() == set_two->order()
      && set_one->index() < set_two->index()) std::swap(set_one, set_two);
  result = Zbdd::Apply(type, set_one, set_two, limit_order);
  return result;
}

VertexPtr Zbdd::Apply(Operator type, const TerminalPtr& term_one,
                      const VertexPtr& arg_two) noexcept {
  switch (type) {
    case kOrGate:
      if (term_one->value()) return kBase_;
      return arg_two;
    case kAndGate:
      if (!term_one->value()) return kEmpty_;
      return arg_two;
    default:
      assert(false && "Unsupported Boolean operation on ZBDD.");
  }
}

VertexPtr Zbdd::Apply(Operator type, const SetNodePtr& arg_one,
                      const SetNodePtr& arg_two, int limit_order) noexcept {
  VertexPtr high;
  VertexPtr low;
  int limit_high = limit_order - 1;
  if (arg_one->index() < 0 || arg_one->module() || this->IsGate(arg_one))
    ++limit_high;  // Conservative.
  if (arg_one->order() == arg_two->order() &&
      arg_one->index() == arg_two->index()) {  // The same variable.
    switch (type) {
      case kOrGate:
        high =
            Zbdd::Apply(kOrGate, arg_one->high(), arg_two->high(), limit_high);
        low = Zbdd::Apply(kOrGate, arg_one->low(), arg_two->low(), limit_order);
        break;
      case kAndGate:
        // (x*f1 + f0) * (x*g1 + g0) = x*(f1*(g1 + g0) + f0*g1) + f0*g0
        high = Zbdd::Apply(
            kOrGate,
            Zbdd::Apply(kAndGate, arg_one->high(),
                        Zbdd::Apply(kOrGate, arg_two->high(),
                                    arg_two->low(), limit_high),
                        limit_high),
            Zbdd::Apply(kAndGate, arg_one->low(), arg_two->high(), limit_high),
            limit_high);
        low =
            Zbdd::Apply(kAndGate, arg_one->low(), arg_two->low(), limit_order);
        break;
      default:
        assert(false && "Unsupported Boolean operation on ZBDD.");
    }
  } else {
    assert((arg_one->order() < arg_two->order() ||
            arg_one->index() > arg_two->index()) &&
           "Ordering contract failed.");
    switch (type) {
      case kOrGate:
        if (arg_one->order() == arg_two->order()) {
          if (arg_one->high()->terminal() && arg_two->high()->terminal())
            return kBase_;
        }
        high = arg_one->high();
        low = Zbdd::Apply(kOrGate, arg_one->low(), arg_two, limit_order);
        break;
      case kAndGate:
        if (arg_one->order() == arg_two->order()) {
          // (x*f1 + f0) * (~x*g1 + g0) = x*f1*g0 + f0*(~x*g1 + g0)
          high = Zbdd::Apply(kAndGate, arg_one->high(), arg_two->low(),
                             limit_high);
        } else {
          high = Zbdd::Apply(kAndGate, arg_one->high(), arg_two, limit_high);
        }
        low = Zbdd::Apply(kAndGate, arg_one->low(), arg_two, limit_order);
        break;
      default:
        assert(false && "Unsupported Boolean operation on ZBDD.");
    }
  }
  if (!high->terminal() && SetNode::Ptr(high)->order() == arg_one->order()) {
    assert(SetNode::Ptr(high)->index() < arg_one->index());
    high = SetNode::Ptr(high)->low();
  }
  if (high->id() == low->id()) return low;
  if (high->terminal() && Terminal::Ptr(high)->value() == false) return low;
  return Zbdd::Minimize(Zbdd::FetchUniqueTable(arg_one, high, low));
}

VertexPtr Zbdd::EliminateComplements(
    const VertexPtr& vertex,
    std::unordered_map<int, VertexPtr>* wide_results) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = (*wide_results)[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  result = Zbdd::EliminateComplement(
      node,
      Zbdd::EliminateComplements(node->high(), wide_results),
      Zbdd::EliminateComplements(node->low(), wide_results));
  return result;
}

VertexPtr Zbdd::EliminateComplement(const SetNodePtr& node,
                                    const VertexPtr& high,
                                    const VertexPtr& low) noexcept {
  if (high->id() == low->id()) return low;
  if (high->terminal() && Terminal::Ptr(high)->value() == false) return low;
  if (node->index() < 0)  /// @todo Consider tracking the order.
    return Zbdd::Apply(kOrGate, high, low, kSettings_.limit_order());
  return Zbdd::Minimize(Zbdd::FetchUniqueTable(node, high, low));
}

VertexPtr Zbdd::EliminateConstantModules(
    const VertexPtr& vertex,
    std::unordered_map<int, VertexPtr>* results) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = (*results)[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  result = Zbdd::EliminateConstantModule(
      node,
      Zbdd::EliminateConstantModules(node->high(), results),
      Zbdd::EliminateConstantModules(node->low(), results));
  return result;
}

VertexPtr Zbdd::EliminateConstantModule(const SetNodePtr& node,
                                        const VertexPtr& high,
                                        const VertexPtr& low) noexcept {
  if (high->id() == low->id()) return low;
  if (high->terminal() && Terminal::Ptr(high)->value() == false) return low;
  if (node->module()) {
    VertexPtr& module = modules_.find(node->index())->second;
    if (module->terminal()) {
      if (!Terminal::Ptr(module)->value()) return low;
      return Zbdd::Apply(kOrGate, high, low, kSettings_.limit_order());
    }
  }
  return Zbdd::Minimize(Zbdd::FetchUniqueTable(node, high, low));
}

VertexPtr Zbdd::Minimize(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return vertex;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->minimal()) return vertex;
  VertexPtr& result = minimal_results_[vertex->id()];
  if (result) return result;
  VertexPtr high = Zbdd::Minimize(node->high());
  VertexPtr low = Zbdd::Minimize(node->low());
  high = Zbdd::Subsume(high, low);
  assert(high->id() != low->id() && "Subsume failed!");
  if (high->terminal() && !Terminal::Ptr(high)->value()) {  // Reduction rule.
    result = low;
    return result;
  }
  result = Zbdd::FetchUniqueTable(node, high, low);
  SetNode::Ptr(result)->minimal(true);
  return result;
}

VertexPtr Zbdd::Subsume(const VertexPtr& high, const VertexPtr& low) noexcept {
  if (low->terminal()) return Terminal::Ptr(low)->value() ? kEmpty_ : high;
  if (high->terminal()) return high;  // No need to reduce terminal sets.
  VertexPtr& computed = subsume_table_[{high->id(), low->id()}];
  if (computed) return computed;

  SetNodePtr high_node = SetNode::Ptr(high);
  SetNodePtr low_node = SetNode::Ptr(low);
  if (high_node->order() > low_node->order() ||
      (high_node->order() == low_node->order() &&
       high_node->index() < low_node->index())) {
    computed = Zbdd::Subsume(high, low_node->low());
    return computed;
  }
  VertexPtr subhigh;
  VertexPtr sublow;
  if (high_node->order() == low_node->order() &&
      high_node->index() == low_node->index()) {
    assert(high_node->index() == low_node->index());
    subhigh = Zbdd::Subsume(high_node->high(), low_node->high());
    subhigh = Zbdd::Subsume(subhigh, low_node->low());
    sublow = Zbdd::Subsume(high_node->low(), low_node->low());
  } else {
    assert(high_node->order() < low_node->order() ||
           (high_node->order() == low_node->order() &&
            high_node->index() > low_node->index()));
    subhigh = Zbdd::Subsume(high_node->high(), low);
    sublow = Zbdd::Subsume(high_node->low(), low);
  }
  if (subhigh->terminal() && !Terminal::Ptr(subhigh)->value()) {
    computed = sublow;
    return computed;
  }
  assert(subhigh->id() != sublow->id());
  SetNodePtr new_high = Zbdd::FetchUniqueTable(high_node, subhigh, sublow);
  new_high->minimal(high_node->minimal());
  computed = new_high;
  return computed;
}

void Zbdd::GatherModules(const VertexPtr& vertex,
                         std::vector<int>* modules) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->module()) {
    auto it = std::find(modules->begin(), modules->end(), node->index());
    if (it == modules->end()) modules->push_back(node->index());
  }
  Zbdd::GatherModules(node->high(), modules);
  Zbdd::GatherModules(node->low(), modules);
}

std::vector<std::vector<int>>
Zbdd::GenerateProducts(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) {
    if (Terminal::Ptr(vertex)->value()) return {{}};  // The Base set signature.
    return {};  // Don't include 0/NULL sets.
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  assert(node->minimal() && "Detected non-minimal ZBDD.");
  if (node->mark()) return node->products();
  node->mark(true);
  std::vector<Product> low = Zbdd::GenerateProducts(node->low());
  std::vector<Product> high = Zbdd::GenerateProducts(node->high());
  auto& result = low;  // For clarity.
  if (node->module()) {
    VertexPtr module_vertex = modules_.find(node->index())->second;  // Extra.
    std::vector<Product> module = Zbdd::GenerateProducts(module_vertex);
    for (auto& product : high) {  // Cross-product.
      for (auto& module_set : module) {
        if (product.size() + module_set.size() > kSettings_.limit_order())
          continue;  // Cut-off on the product size.
        Product combo = product;
        combo.insert(combo.end(), module_set.begin(), module_set.end());
        result.emplace_back(std::move(combo));
      }
    }
  } else {
    for (auto& product : high) {
      if (product.size() == kSettings_.limit_order()) continue;
      product.push_back(node->index());
      result.emplace_back(std::move(product));
    }
  }

  // Destroy the subgraph to remove extra reference counts.
  node->CutBranches();

  if (node.use_count() > 2) node->products(result);
  return result;
}

int Zbdd::CountSetNodes(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 0;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return 0;
  node->mark(true);
  int in_module = 0;
  if (node->module()) {
    in_module = Zbdd::CountSetNodes(modules_.find(node->index())->second);
  }
  return 1 + in_module + Zbdd::CountSetNodes(node->high()) +
         Zbdd::CountSetNodes(node->low());
}

int64_t Zbdd::CountProducts(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) {
    if (Terminal::Ptr(vertex)->value()) return 1;
    return 0;
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return node->count();
  node->mark(true);
  int64_t multiplier = 1;  // Multiplier of the module.
  if (node->module()) {
    VertexPtr module = modules_.find(node->index())->second;
    multiplier = Zbdd::CountProducts(module);
  }
  node->count(multiplier * Zbdd::CountProducts(node->high()) +
              Zbdd::CountProducts(node->low()));
  return node->count();
}

void Zbdd::ClearMarks(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (!node->mark()) return;
  node->mark(false);
  if (node->module()) {
    Zbdd::ClearMarks(modules_.find(node->index())->second);
  }
  Zbdd::ClearMarks(node->high());
  Zbdd::ClearMarks(node->low());
}

void Zbdd::TestStructure(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return;
  node->mark(true);
  assert(node->index() && "Illegal index for a node.");
  assert(node->order() && "Improper order for nodes.");
  assert(node->high() && node->low() && "Malformed node high/low pointers.");
  assert(!(node->high()->terminal() && !Terminal::Ptr(node->high())->value())
         && "Reduction rule failure.");
  assert((node->high()->id() != node->low()->id()) && "Minimization failure.");
  assert(!(!node->high()->terminal() &&
           node->order() >= SetNode::Ptr(node->high())->order()) &&
         "Ordering of nodes failed.");
  assert(!(!node->low()->terminal() &&
           node->order() > SetNode::Ptr(node->low())->order()) &&
         "Ordering of nodes failed.");
  assert(!(!node->low()->terminal() &&
           node->order() == SetNode::Ptr(node->low())->order() &&
           node->index() <= SetNode::Ptr(node->low())->index()) &&
         "Ordering of complements failed.");
  assert(!(!node->high()->terminal() && node->minimal() &&
           !SetNode::Ptr(node->high())->minimal()) &&
         "Non-minimal branches in minimal ZBDD.");
  assert(!(!node->low()->terminal() && node->minimal() &&
           !SetNode::Ptr(node->low())->minimal()) &&
         "Non-minimal branches in minimal ZBDD.");
  if (node->module()) {
    VertexPtr module = modules_.find(node->index())->second;
    assert(!module->terminal() && "Terminal modules must be removed.");
    Zbdd::TestStructure(module);
  }
  Zbdd::TestStructure(node->high());
  Zbdd::TestStructure(node->low());
}

namespace zbdd {

CutSetContainer::CutSetContainer(const Settings& settings,
                                 int gate_index_bound) noexcept
    : Zbdd::Zbdd(settings),
      gate_index_bound_(gate_index_bound) {
  root_ = kEmpty_;  // Empty container.
}

VertexPtr CutSetContainer::ConvertGate(const IGatePtr& gate) noexcept {
  assert(gate->type() == kAndGate || gate->type() == kOrGate);
  assert(gate->constant_args().empty());
  assert(gate->args().size() > 1);
  std::vector<SetNodePtr> args;
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    args.push_back(Zbdd::FetchUniqueTable(arg.first, kBase_, kEmpty_,
                                          arg.second->order(), false));
  }
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    assert(arg.first > 0 && "Complements must be pushed down to variables.");
    args.push_back(Zbdd::FetchUniqueTable(arg.first, kBase_, kEmpty_,
                                          arg.second->order(),
                                          arg.second->IsModule()));
  }
  std::sort(args.begin(), args.end(),
            [](const SetNodePtr& lhs, const SetNodePtr& rhs) {
    return lhs->order() > rhs->order();
  });
  auto it = args.cbegin();
  VertexPtr result = *it;
  for (++it; it != args.cend(); ++it) {
    result = Zbdd::Apply(gate->type(), result, *it, kSettings_.limit_order());
  }
  return result;
}

VertexPtr CutSetContainer::ExtractIntermediateCutSets(int index) noexcept {
  assert(index && index > gate_index_bound_);
  assert(!root_->terminal() && "Impossible to have intermediate cut sets.");
  assert(index == SetNode::Ptr(root_)->index() && "Broken ordering!");
  LOG(DEBUG5) << "Extracting cut sets for G" << index;
  SetNodePtr node = SetNode::Ptr(root_);
  root_ = node->low();
  return node->high();
}

VertexPtr CutSetContainer::ExpandGate(const VertexPtr& gate_zbdd,
                                      const VertexPtr& cut_sets) noexcept {
  return Zbdd::Apply(kAndGate, gate_zbdd, cut_sets, kSettings_.limit_order());
}

void CutSetContainer::Merge(const VertexPtr& vertex) noexcept {
  root_ = Zbdd::Apply(kOrGate, root_, vertex, kSettings_.limit_order());
  and_table_.clear();
  or_table_.clear();
  subsume_table_.clear();
  minimal_results_.clear();
}

void CutSetContainer::EliminateComplements() noexcept {
  std::unordered_map<int, VertexPtr> wide_results;
  root_ = Zbdd::EliminateComplements(root_, &wide_results);
}

void CutSetContainer::EliminateConstantModules() noexcept {
  std::unordered_map<int, VertexPtr> results;
  root_ = Zbdd::EliminateConstantModules(root_, &results);
}

std::vector<int> CutSetContainer::GatherModules() noexcept {
  assert(modules_.empty() && "Unexpected call with defined modules?!");
  std::vector<int> modules;
  Zbdd::GatherModules(root_, &modules);
  return modules;
}

void CutSetContainer::JoinModule(int index,
                                 const CutSetContainer& container) noexcept {
  assert(!modules_.count(index));
  assert(container.root_->terminal() ||
         SetNode::Ptr(container.root_)->minimal());
  modules_.emplace(index, container.root_);
  modules_.insert(container.modules_.begin(), container.modules_.end());
}

}  // namespace zbdd

}  // namespace scram
