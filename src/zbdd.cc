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
  int64_t number = Zbdd::CountCutSets(root_);
  LOG(DEBUG3) << "There are " << number << " cut sets in total.";
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
