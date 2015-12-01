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

/// @file zbdd.cc
/// Implementation of Zero-Suppressed BDD algorithms.

#include "zbdd.h"

#include "logger.h"

namespace scram {

Zbdd::Zbdd(const Settings& settings) noexcept
    : kSettings_(settings),
      kBase_(std::make_shared<Terminal>(true)),
      kEmpty_(std::make_shared<Terminal>(false)),
      set_id_(2) {}

#define LOG_ZBDD                                                             \
  LOG(DEBUG4) << "# of ZBDD nodes created: " << set_id_ - 1;                 \
  LOG(DEBUG4) << "# of entries in unique table: " << unique_table_.size();   \
  LOG(DEBUG4) << "# of entries in AND table: " << and_table_.size();         \
  LOG(DEBUG4) << "# of entries in OR table: " << or_table_.size();           \
  LOG(DEBUG4) << "# of entries in subsume table: " << subsume_table_.size(); \
  Zbdd::ClearMarks(root_);                                                   \
  LOG(DEBUG4) << "# of SetNodes in ZBDD: " << Zbdd::CountSetNodes(root_);    \
  Zbdd::ClearMarks(root_);                                                   \
  LOG(DEBUG3) << "There are " << Zbdd::CountCutSets(root_) << " cut sets.";  \
  Zbdd::ClearMarks(root_)

Zbdd::Zbdd(const Bdd* bdd, const Settings& settings) noexcept
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
    std::unordered_map<int, VertexPtr> gates;
    root_ = Zbdd::ConvertGraph(fault_tree->root(), &gates);
    if (!fault_tree->coherent()) {
      Zbdd::ClearMarks(root_);
      Zbdd::TestStructure(root_);
      Zbdd::ClearMarks(root_);
      LOG(DEBUG5) << "Eliminating complements from ZBDD...";
      std::unordered_map<int, VertexPtr> wide_results;
      root_ = Zbdd::EliminateComplements(root_, &wide_results);
      LOG(DEBUG5) << "Finished complement elimination.";
    }
  }
  Zbdd::ClearMarks(root_);
  Zbdd::TestStructure(root_);
  LOG_ZBDD;
  LOG(DEBUG2) << "Created ZBDD from Boolean Graph in " << DUR(init_time);
}

Zbdd::Zbdd(int root_index,
           const std::vector<std::pair<int, mocus::CutSetContainer>>& cut_sets,
           const Settings& settings) noexcept
    : Zbdd::Zbdd(settings) {
  CLOCK(init_time);
  LOG(DEBUG2) << "Creating ZBDD from cut sets...";
  for (auto it = cut_sets.rbegin(); it != cut_sets.rend(); ++it) {
    assert(!modules_.count(it->first) && "Repeated calculation of modules.");
    modules_.emplace(it->first, Zbdd::ConvertCutSets(it->second));
  }
  root_ = modules_.find(root_index)->second;
  Zbdd::ClearMarks(root_);
  Zbdd::TestStructure(root_);
  LOG_ZBDD;
  LOG(DEBUG2) << "Created ZBDD from cut sets in " << DUR(init_time);
}

void Zbdd::Analyze() noexcept {
  CLOCK(analysis_time);
  LOG(DEBUG2) << "Analyzing ZBDD...";

  CLOCK(minimize_time);
  LOG(DEBUG3) << "Minimizing ZBDD...";
  std::unordered_map<int, VertexPtr> minimal_results;
  root_ = Zbdd::Minimize(root_, &minimal_results);
  minimal_results.clear();
  Zbdd::ClearMarks(root_);
  Zbdd::TestStructure(root_);
  LOG_ZBDD;
  LOG(DEBUG3) << "Finished ZBDD minimization in " << DUR(minimize_time);

  // Complete cleanup of the memory.
  unique_table_.clear();
  and_table_.clear();
  or_table_.clear();
  subsume_table_.clear();

  CLOCK(gen_time);
  LOG(DEBUG3) << "Getting cut sets from minimized ZBDD...";
  cut_sets_ = Zbdd::GenerateCutSets(root_);

  // Cleanup of temporary cut sets.
  modules_.clear();
  root_ = kBase_;

  LOG(DEBUG3) << cut_sets_.size() << " cut sets are found in " << DUR(gen_time);
  LOG(DEBUG2) << "Finished ZBDD analysis in " << DUR(analysis_time);
}

#undef LOG_ZBDD

const SetNodePtr& Zbdd::FetchUniqueTable(int index, const VertexPtr& high,
                                         const VertexPtr& low, int order,
                                         bool module) noexcept {
  SetNodePtr& in_table = unique_table_[{index, high->id(), low->id()}];
  if (in_table) return in_table;
  assert(order > 0 && "Improper order.");
  in_table = std::make_shared<SetNode>(index, order);
  in_table->id(set_id_++);
  in_table->module(module);
  in_table->high(high);
  in_table->low(low);
  return in_table;
}

VertexPtr Zbdd::ConvertBdd(const VertexPtr& vertex, bool complement,
                           const Bdd* bdd_graph, int limit_order,
                           PairTable<VertexPtr>* ites) noexcept {
  if (vertex->terminal()) return complement ? kEmpty_ : kBase_;
  int sign = complement ? -1 : 1;
  VertexPtr& result = (*ites)[{sign * vertex->id(), limit_order}];
  if (result) return result;
  ItePtr ite = Ite::Ptr(vertex);
  VertexPtr low =
      Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                       bdd_graph, limit_order, ites);
  if (limit_order == 0) {  // Cut-off on the cut set size.
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
      assert(!Terminal::Ptr(module_set)->value());
      result = low;
      return result;
    }
  }
  int limit_high = limit_order - 1;  // Requested order for the high node.
  if (ite->module()) limit_high += 1;  // Conservative approach.
  VertexPtr high =
      Zbdd::ConvertBdd(ite->high(), complement, bdd_graph, limit_high, ites);
  if ((high->terminal() && !Terminal::Ptr(high)->value()) ||
      (high->id() == low->id()) ||
      (low->terminal() && Terminal::Ptr(low)->value())) {
    result = low;  // Reduce and minimize.
    return result;
  }
  result = Zbdd::FetchUniqueTable(ite->index(), high, low, ite->order(),
                                  ite->module());
  return result;
}

VertexPtr Zbdd::ConvertGraph(
    const IGatePtr& gate,
    std::unordered_map<int, VertexPtr>* gates) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  VertexPtr& result = (*gates)[gate->index()];
  if (result) return result;
  std::vector<VertexPtr> args;
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    args.push_back(Zbdd::FetchUniqueTable(arg.first, kBase_, kEmpty_,
                                          arg.second->order(), false));
  }
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    assert(arg.first > 0 && "Complements must be pushed down to variables.");
    VertexPtr res = Zbdd::ConvertGraph(arg.second, gates);
    if (arg.second->IsModule()) {
      args.push_back(Zbdd::FetchUniqueTable(arg.first, kBase_, kEmpty_,
                                            arg.second->order(), true));
      modules_.emplace(arg.second->index(), res);
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
  assert(result);
  return result;
}

VertexPtr Zbdd::ConvertCutSets(
    const mocus::CutSetContainer& cut_sets) noexcept {
  std::vector<mocus::CutSetPtr> data(cut_sets.begin(), cut_sets.end());
  std::sort(data.begin(), data.end(),
            [](const mocus::CutSetPtr& lhs, const mocus::CutSetPtr& rhs) {
    return lhs->size() < rhs->size();
  });
  if (data.empty()) return kEmpty_;
  if (data.front()->empty()) return kBase_;

  VertexPtr result = kEmpty_;
  for (const auto& cut_set : data) {
    result = Zbdd::Apply(kOrGate, result, Zbdd::EmplaceCutSet(cut_set),
                         kSettings_.limit_order());
  }
  return result;
}

VertexPtr Zbdd::EmplaceCutSet(const mocus::CutSetPtr& cut_set) noexcept {
  assert(!cut_set->empty() && "Unity cut set must be sanitized.");
  VertexPtr result = kBase_;
  std::vector<int>::const_reverse_iterator it;
  for (it = cut_set->modules().rbegin(); it != cut_set->modules().rend();
       ++it) {
    int index = *it;
    const VertexPtr& module = modules_.find(index)->second;
    if (module->terminal()) {
      if (!Terminal::Ptr(module)->value()) return kEmpty_;
      continue;  // The result does not change for the TRUE module.
    }
    result = Zbdd::FetchUniqueTable(index, result, kEmpty_, index + 1, true);
  }
  for (it = cut_set->literals().rbegin(); it != cut_set->literals().rend();
       ++it) {
    int index = *it;
    result = Zbdd::FetchUniqueTable(index, result, kEmpty_, index + 1, false);
  }
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
  if (arg_one->terminal() && arg_two->terminal())
    return Zbdd::Apply(type, Terminal::Ptr(arg_one), Terminal::Ptr(arg_two));
  if (arg_one->terminal())
    return Zbdd::Apply(type, SetNode::Ptr(arg_two), Terminal::Ptr(arg_one));
  if (arg_two->terminal())
    return Zbdd::Apply(type, SetNode::Ptr(arg_one), Terminal::Ptr(arg_two));

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
                      const TerminalPtr& term_two) noexcept {
  switch (type) {
    case kOrGate:
      if (term_one->value() || term_two->value()) return kBase_;
      return kEmpty_;
    case kAndGate:
      if (!term_one->value() || !term_two->value()) return kEmpty_;
      return kBase_;
    default:
      assert(false && "Unsupported Boolean operation on ZBDD.");
  }
}

VertexPtr Zbdd::Apply(Operator type, const SetNodePtr& set_node,
                      const TerminalPtr& term) noexcept {
  switch (type) {
    case kOrGate:
      if (term->value()) return kBase_;
      return set_node;
    case kAndGate:
      if (!term->value()) return kEmpty_;
      return set_node;
    default:
      assert(false && "Unsupported Boolean operation on ZBDD.");
  }
}

VertexPtr Zbdd::Apply(Operator type, const SetNodePtr& arg_one,
                      const SetNodePtr& arg_two, int limit_order) noexcept {
  VertexPtr high;
  VertexPtr low;
  int limit_high = limit_order - 1;
  if (arg_one->index() < 0 || arg_one->module()) ++limit_high;
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
  if (high->id() == low->id()) return low;
  if (high->terminal() && Terminal::Ptr(high)->value() == false) return low;
  return Zbdd::FetchUniqueTable(arg_one->index(), high, low, arg_one->order(),
                                arg_one->module());
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
      Zbdd::EliminateComplements(node->low(), wide_results),
      wide_results);
  return result;
}

VertexPtr Zbdd::EliminateComplement(
    const SetNodePtr& node,
    const VertexPtr& high,
    const VertexPtr& low,
    std::unordered_map<int, VertexPtr>* wide_results) noexcept {
  if (node->index() < 0)  /// @todo Consider tracking the order.
    return Zbdd::Apply(kOrGate, high, low, kSettings_.limit_order());
  if (high->id() == low->id()) return low;
  if (high->terminal() && Terminal::Ptr(high)->value() == false) return low;

  if (node->module()) {
    VertexPtr& module = modules_.find(node->index())->second;
    module = Zbdd::EliminateComplements(module, wide_results);
    if (module->terminal()) {
      if (!Terminal::Ptr(module)->value()) return low;
      return Zbdd::Apply(kOrGate, high, low, kSettings_.limit_order());
    }
  }
  return Zbdd::FetchUniqueTable(node->index(), high, low, node->order(),
                                node->module());
}

VertexPtr Zbdd::Minimize(
    const VertexPtr& vertex,
    std::unordered_map<int, VertexPtr>* minimal_results) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = (*minimal_results)[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->module()) {
    VertexPtr& module = modules_.find(node->index())->second;
    module = Zbdd::Minimize(module, minimal_results);
  }
  VertexPtr high = Zbdd::Minimize(node->high(), minimal_results);
  VertexPtr low = Zbdd::Minimize(node->low(), minimal_results);
  high = Zbdd::Subsume(high, low);
  assert(high->id() != low->id() && "Subsume failed!");
  if (high->terminal() && !Terminal::Ptr(high)->value()) {  // Reduction rule.
    result = low;
    return result;
  }
  result = Zbdd::FetchUniqueTable(node->index(), high, low, node->order(),
                                  node->module());
  return result;
}

VertexPtr Zbdd::Subsume(const VertexPtr& high, const VertexPtr& low) noexcept {
  if (low->terminal()) return Terminal::Ptr(low)->value() ? kEmpty_ : high;
  if (high->terminal()) return high;  // No need to reduce terminal sets.
  VertexPtr& computed = subsume_table_[{high->id(), low->id()}];
  if (computed) return computed;

  SetNodePtr high_node = SetNode::Ptr(high);
  SetNodePtr low_node = SetNode::Ptr(low);
  if (high_node->order() > low_node->order()) {
    computed = Zbdd::Subsume(high, low_node->low());
    return computed;
  }
  VertexPtr subhigh;
  VertexPtr sublow;
  if (high_node->order() == low_node->order()) {
    assert(high_node->index() == low_node->index());
    subhigh = Zbdd::Subsume(high_node->high(), low_node->high());
    subhigh = Zbdd::Subsume(subhigh, low_node->low());
    sublow = Zbdd::Subsume(high_node->low(), low_node->low());
  } else {
    assert(high_node->order() < low_node->order());
    subhigh = Zbdd::Subsume(high_node->high(), low);
    sublow = Zbdd::Subsume(high_node->low(), low);
  }
  if (subhigh->terminal() && !Terminal::Ptr(subhigh)->value()) {
    computed = sublow;
    return computed;
  }
  assert(subhigh->id() != sublow->id());
  computed = Zbdd::FetchUniqueTable(high_node->index(), subhigh, sublow,
                                    high_node->order(), high_node->module());
  return computed;
}

std::vector<std::vector<int>>
Zbdd::GenerateCutSets(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) {
    if (Terminal::Ptr(vertex)->value()) return {{}};  // The Base set signature.
    return {};  // Don't include 0/NULL sets.
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return node->cut_sets();
  node->mark(true);
  std::vector<CutSet> low = Zbdd::GenerateCutSets(node->low());
  std::vector<CutSet> high = Zbdd::GenerateCutSets(node->high());
  auto& result = low;  // For clarity.
  if (node->module()) {
    VertexPtr module_vertex = modules_.find(node->index())->second;  // Extra.
    std::vector<CutSet> module = Zbdd::GenerateCutSets(module_vertex);
    for (auto& cut_set : high) {  // Cross-product.
      for (auto& module_set : module) {
        if (cut_set.size() + module_set.size() > kSettings_.limit_order())
          continue;  // Cut-off on the cut set size.
        CutSet combo = cut_set;
        combo.insert(combo.end(), module_set.begin(), module_set.end());
        result.emplace_back(std::move(combo));
      }
    }
  } else {
    for (auto& cut_set : high) {
      if (cut_set.size() == kSettings_.limit_order()) continue;
      cut_set.push_back(node->index());
      result.emplace_back(std::move(cut_set));
    }
  }

  // Destroy the subgraph to remove extra reference counts.
  node->low(kEmpty_);
  node->high(kBase_);

  if (node.use_count() > 2) node->cut_sets(result);
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

int64_t Zbdd::CountCutSets(const VertexPtr& vertex) noexcept {
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
    multiplier = Zbdd::CountCutSets(module);
  }
  node->count(multiplier * Zbdd::CountCutSets(node->high()) +
              Zbdd::CountCutSets(node->low()));
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
           node->order() >= SetNode::Ptr(node->low())->order()) &&
         "Ordering of nodes failed.");
  if (node->module()) {
    VertexPtr module = modules_.find(node->index())->second;
    assert(!module->terminal() && "Terminal modules must be removed.");
    Zbdd::TestStructure(module);
  }
  Zbdd::TestStructure(node->high());
  Zbdd::TestStructure(node->low());
}

}  // namespace scram
