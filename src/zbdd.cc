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

Zbdd::Zbdd(const Bdd* bdd, const Settings& settings) noexcept
    : Zbdd::Zbdd(settings) {
  CLOCK(init_time);
  LOG(DEBUG2) << "Creating ZBDD from BDD...";
  const Bdd::Function& bdd_root = bdd->root();
  root_ = Zbdd::ConvertBdd(bdd_root.vertex, bdd_root.complement, bdd,
                           kSettings_.limit_order());
  LOG(DEBUG4) << "# of ZBDD nodes created: " << set_id_ - 1;
  LOG(DEBUG4) << "# of SetNodes in ZBDD: " << Zbdd::CountSetNodes(root_);
  LOG(DEBUG2) << "Created ZBDD from BDD in " << DUR(init_time);

  Zbdd::ClearMarks(root_);
  LOG(DEBUG3) << "There are " << Zbdd::CountCutSets(root_) << " cut sets.";
  Zbdd::ClearMarks(root_);
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
  } else {
    root_ = Zbdd::ConvertGraph(fault_tree->root());
  }
  LOG(DEBUG4) << "# of ZBDD nodes created: " << set_id_ - 1;
  LOG(DEBUG4) << "# of SetNodes in ZBDD: " << Zbdd::CountSetNodes(root_);
  LOG(DEBUG2) << "Created ZBDD from BDD in " << DUR(init_time);

  Zbdd::ClearMarks(root_);
  LOG(DEBUG3) << "There are " << Zbdd::CountCutSets(root_) << " cut sets.";
  Zbdd::ClearMarks(root_);
}

void Zbdd::Analyze() noexcept {
  CLOCK(analysis_time);
  LOG(DEBUG2) << "Analyzing ZBDD...";

  CLOCK(minimize_time);
  LOG(DEBUG3) << "Minimizing ZBDD...";
  root_ = Zbdd::Minimize(root_);
  LOG(DEBUG3) << "Finished ZBDD minimization in " << DUR(minimize_time);
  Zbdd::ClearMarks(root_);
  LOG(DEBUG4) << "# of ZBDD nodes created: " << set_id_ - 1;
  LOG(DEBUG4) << "# of SetNodes in ZBDD: " << Zbdd::CountSetNodes(root_);
  Zbdd::ClearMarks(root_);
  int64_t number = Zbdd::CountCutSets(root_);
  Zbdd::ClearMarks(root_);
  LOG(DEBUG3) << "There are " << number << " cut sets in total.";

  CLOCK(gen_time);
  LOG(DEBUG3) << "Getting cut sets from minimized ZBDD...";
  cut_sets_ = Zbdd::GenerateCutSets(root_);
  Zbdd::ClearMarks(root_);
  LOG(DEBUG3) << cut_sets_.size() << " cut sets are found in " << DUR(gen_time);
  LOG(DEBUG2) << "Finished ZBDD analysis in " << DUR(analysis_time);
}

std::shared_ptr<Vertex> Zbdd::ConvertBdd(const VertexPtr& vertex,
                                         bool complement,
                                         const Bdd* bdd_graph,
                                         int limit_order) noexcept {
  if (vertex->terminal()) return complement ? kEmpty_ : kBase_;
  if (limit_order == 0) return kEmpty_;  // Cut-off on the cut set size.
  int sign = complement ? -1 : 1;
  VertexPtr& result = ites_[{sign * vertex->id(), limit_order}];
  if (result) return result;
  ItePtr ite = Ite::Ptr(vertex);
  SetNodePtr zbdd = std::make_shared<SetNode>(ite->index(), ite->order());
  int limit_high = limit_order - 1;  // Requested order for the high node.
  if (ite->module()) {  // This is a proxy and not a variable.
    zbdd->module(true);
    const Bdd::Function& module =
        bdd_graph->gates().find(ite->index())->second;
    VertexPtr module_set =
        Zbdd::ConvertBdd(module.vertex, module.complement,
                         bdd_graph, kSettings_.limit_order());
    limit_high += 1;  // Conservative approach.
    modules_.emplace(ite->index(), module_set);
  }
  zbdd->high(Zbdd::ConvertBdd(ite->high(), complement, bdd_graph, limit_high));
  zbdd->low(Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                             bdd_graph, limit_order));
  if ((zbdd->high()->terminal() && !Terminal::Ptr(zbdd->high())->value()) ||
      (zbdd->high()->id() == zbdd->low()->id()) ||
      (zbdd->low()->terminal() && Terminal::Ptr(zbdd->low())->value())) {
    result = zbdd->low();  // Reduce and minimize.
    return result;
  }
  SetNodePtr& in_table =
      unique_table_[{zbdd->index(), zbdd->high()->id(), zbdd->low()->id()}];
  if (!in_table) {
    in_table = zbdd;
    zbdd->id(set_id_++);
  }
  result = in_table;
  return result;
}

std::shared_ptr<Vertex> Zbdd::ConvertGraph(const IGatePtr& gate) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  VertexPtr& result = gates_[gate->index()];
  if (result) return result;
  std::vector<VertexPtr> args;
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    assert(arg.first > 0 && "Cannot handle non-coherent graphs.");
    args.push_back(Zbdd::ConvertGraph(arg.second));
  }
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    assert(arg.first > 0 && "Cannot handle non-coherent graphs.");
    VertexPtr res = Zbdd::ConvertGraph(arg.second);
    if (arg.second->IsModule()) {
      args.push_back(Zbdd::CreateModuleProxy(arg.second));
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
    result = Zbdd::Apply(gate->type(), result, *it);
  }
  assert(result);
  return result;
}

std::shared_ptr<SetNode> Zbdd::ConvertGraph(
    const VariablePtr& variable) noexcept {
  SetNodePtr& in_table = unique_table_[{variable->index(), 1, 0}];
  if (in_table) return in_table;
  in_table = std::make_shared<SetNode>(variable->index(), variable->order());
  in_table->id(set_id_++);
  in_table->high(kBase_);
  in_table->low(kEmpty_);
  return in_table;
}

std::shared_ptr<SetNode> Zbdd::CreateModuleProxy(
    const IGatePtr& gate) noexcept {
  assert(gate->IsModule());
  SetNodePtr& in_table = unique_table_[{gate->index(), 1, 0}];
  if (in_table) return in_table;
  in_table = std::make_shared<SetNode>(gate->index(), gate->order());
  in_table->module(true);  // The main difference.
  in_table->id(set_id_++);
  in_table->high(kBase_);
  in_table->low(kEmpty_);
  return in_table;
}

std::shared_ptr<Vertex> Zbdd::Apply(Operator type, const VertexPtr& arg_one,
                                    const VertexPtr& arg_two) noexcept {
  if (arg_one->terminal() && arg_two->terminal())
    return Zbdd::Apply(type, Terminal::Ptr(arg_one), Terminal::Ptr(arg_two));
  if (arg_one->terminal())
    return Zbdd::Apply(type, SetNode::Ptr(arg_two), Terminal::Ptr(arg_one));
  if (arg_two->terminal())
    return Zbdd::Apply(type, SetNode::Ptr(arg_one), Terminal::Ptr(arg_two));

  if (arg_one->id() == arg_two->id()) return arg_one;

  Triplet sig = Zbdd::GetSignature(type, arg_one, arg_two);
  VertexPtr& result = compute_table_[sig];  // Register if not computed.
  if (result) return result;  // Already computed.

  SetNodePtr set_one = SetNode::Ptr(arg_one);
  SetNodePtr set_two = SetNode::Ptr(arg_two);
  if (set_one->order() > set_two->order()) std::swap(set_one, set_two);
  result = Zbdd::Apply(type, set_one, set_two);
  return result;
}

std::shared_ptr<Vertex> Zbdd::Apply(Operator type, const TerminalPtr& term_one,
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

std::shared_ptr<Vertex> Zbdd::Apply(Operator type, const SetNodePtr& set_node,
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

std::shared_ptr<Vertex> Zbdd::Apply(Operator type, const SetNodePtr& arg_one,
                                    const SetNodePtr& arg_two) noexcept {
  VertexPtr high;
  VertexPtr low;
  if (arg_one->order() == arg_two->order()) {  // The same variable.
    assert(arg_one->index() == arg_two->index());
    switch (type) {
      case kOrGate:
        high = Zbdd::Apply(type, arg_one->high(), arg_two->high());
        low = Zbdd::Apply(type, arg_one->low(), arg_two->low());
        break;
      case kAndGate:
        // (x*f1 + f0) * (x*g1 + g0) = x*(f1*(g1 + g0) + f0*g1) + f0*g0
        high = Zbdd::Apply(
            kOrGate,
            Zbdd::Apply(kAndGate, arg_one->high(),
                        Zbdd::Apply(kOrGate, arg_two->high(), arg_two->low())),
            Zbdd::Apply(kAndGate, arg_one->low(), arg_two->high()));
        low = Zbdd::Apply(type, arg_one->low(), arg_two->low());
        break;
      default:
        assert(false && "Unsupported Boolean operation on ZBDD.");
    }
  } else {
    assert(arg_one->order() < arg_two->order());
    switch (type) {
      case kOrGate:
        high = arg_one->high();
        low = Zbdd::Apply(kOrGate, arg_one->low(), arg_two);
        break;
      case kAndGate:
        high = Zbdd::Apply(kAndGate, arg_one->high(), arg_two);
        low = Zbdd::Apply(kAndGate, arg_one->low(), arg_two);
        break;
      default:
        assert(false && "Unsupported Boolean operation on ZBDD.");
    }
  }
  if (high->id() == low->id()) return low;
  if (high->terminal() && Terminal::Ptr(high)->value() == false) return low;

  SetNodePtr& in_table =
      unique_table_[{arg_one->index(), high->id(), low->id()}];
  if (in_table) return in_table;
  in_table = std::make_shared<SetNode>(arg_one->index(), arg_one->order());
  in_table->module(arg_one->module());
  in_table->high(high);
  in_table->low(low);
  in_table->id(set_id_++);
  return in_table;
}

Triplet Zbdd::GetSignature(Operator type, const VertexPtr& arg_one,
                           const VertexPtr& arg_two) noexcept {
  int min_id = std::min(arg_one->id(), arg_two->id());
  int max_id = std::max(arg_one->id(), arg_two->id());
  switch (type) {
    case kOrGate:
      return {min_id, 1, max_id};
    case kAndGate:
      return {min_id, max_id, 0};
    default:
      assert(false && "Only Union and Intersection operations are supported!");
  }
}

std::shared_ptr<Vertex> Zbdd::Minimize(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = minimal_results_[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->module()) {
    VertexPtr& module = modules_.find(node->index())->second;
    module = Zbdd::Minimize(module);
  }
  VertexPtr high = Zbdd::Minimize(node->high());
  VertexPtr low = Zbdd::Minimize(node->low());
  high = Zbdd::Subsume(high, low);
  assert(high->id() != low->id() && "Subsume failed!");
  if (high->terminal() && !Terminal::Ptr(high)->value()) {  // Reduction rule.
    result = low;
    return result;
  }
  SetNodePtr& existing_node =
      unique_table_[{node->index(), high->id(), low->id()}];
  if (!existing_node) {
    existing_node = std::make_shared<SetNode>(node->index(), node->order());
    existing_node->module(node->module());  // Important to transfer.
    existing_node->high(high);
    existing_node->low(low);
    existing_node->id(set_id_++);
  }
  result = existing_node;
  return result;
}

std::shared_ptr<Vertex> Zbdd::Subsume(const VertexPtr& high,
                                      const VertexPtr& low) noexcept {
  if (low->terminal()) {
    if (Terminal::Ptr(low)->value()) {
      return kEmpty_;  // high is always a subset of the Base set.
    } else {
      return high;  // high cannot be a subset of the Empty set.
    }
  }
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
    /// @todo This is correct only for coherent sets.
    subhigh = Zbdd::Subsume(high_node->high(), low_node->high());
    sublow = Zbdd::Subsume(high_node->low(), low_node->low());
  } else {
    assert(high_node->order() < low_node->order());
    subhigh = Zbdd::Subsume(high_node->high(), low);
    sublow = Zbdd::Subsume(high_node->low(), low);
  }
  SetNodePtr& existing_node =
      unique_table_[{high_node->index(), subhigh->id(), sublow->id()}];
  if (!existing_node) {
    existing_node =
        std::make_shared<SetNode>(high_node->index(), high_node->order());
    existing_node->module(high_node->module());
    existing_node->high(subhigh);
    existing_node->low(sublow);
    existing_node->id(set_id_++);
  }
  computed = existing_node;
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
    std::vector<CutSet> module =
        Zbdd::GenerateCutSets(modules_.find(node->index())->second);
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
      result.emplace_back(cut_set);
    }
  }
  node->cut_sets(result);
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

}  // namespace scram
