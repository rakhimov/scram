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

namespace scram {

Zbdd::Zbdd() noexcept
    : kBase_(std::make_shared<Terminal>(true)),
      kEmpty_(std::make_shared<Terminal>(false)),
      set_id_(2) {}

Zbdd::Zbdd(const Bdd* bdd) noexcept : Zbdd::Zbdd() {
  const Bdd::Function& bdd_root = bdd->root();
  root_ = Zbdd::ConvertBdd(bdd_root.vertex, bdd_root.complement, bdd);
}

void Zbdd::Analyze() noexcept {
  root_ = Zbdd::Subsume(root_);
  std::vector<int> seed;
  Zbdd::GenerateCutSets(root_, &seed, &cut_sets_);
}

std::shared_ptr<Vertex> Zbdd::ConvertBdd(const VertexPtr& vertex,
                                         bool complement,
                                         const Bdd* bdd_graph) noexcept {
  if (vertex->terminal()) return complement ? kEmpty_ : kBase_;
  assert(!complement);  // @todo Make it work for non-coherent cases.
  int sign = complement ? -1 : 1;
  SetNodePtr& zbdd = ites_[sign * vertex->id()];
  if (zbdd) return zbdd;
  ItePtr ite = Ite::Ptr(vertex);
  zbdd = std::make_shared<SetNode>(ite->index(), ite->order());
  if (ite->module()) {  // This is a proxy and not a variable.
    zbdd->module(true);
    const Bdd::Function& module =
        bdd_graph->gates().find(ite->index())->second;
    modules_.emplace(
        ite->index(),
        Zbdd::ConvertBdd(module.vertex, module.complement, bdd_graph));
  }
  zbdd->high(Zbdd::ConvertBdd(ite->high(), complement, bdd_graph));
  zbdd->low(Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                             bdd_graph));
  if (zbdd->high()->terminal() && !Terminal::Ptr(zbdd->high())->value())
    return zbdd->low();  // Reduce.
  SetNodePtr& in_table =
      unique_table_[{zbdd->index(), zbdd->high()->id(), zbdd->low()->id()}];
  if (in_table) return in_table;
  in_table = zbdd;
  zbdd->id(set_id_++);
  return zbdd;
}

std::shared_ptr<Vertex> Zbdd::Subsume(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = subsume_results_[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->module()) {
    VertexPtr& module = modules_.find(node->index())->second;
    module = Zbdd::Subsume(module);
  }
  VertexPtr high = Zbdd::Subsume(node->high());
  VertexPtr low = Zbdd::Subsume(node->low());
  high = Zbdd::Subsume(high, low);
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
  /// @todo Define set operation signatures.
  int op = static_cast<int>(SetOp::Without);
  VertexPtr& computed = compute_table_[{op, high->id(), low->id()}];
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

void Zbdd::GenerateCutSets(const VertexPtr& vertex, std::vector<int>* path,
                           std::vector<CutSet>* cut_sets) noexcept {
  if (vertex->terminal()) {
    if (Terminal::Ptr(vertex)->value()) {
      cut_sets->emplace_back(std::move(*path));
    }
    return;  // Don't include 0/NULL sets.
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  std::vector<int> seed(*path);
  Zbdd::GenerateCutSets(node->low(), &seed, cut_sets);

  if (node->module()) {
    std::vector<CutSet>& sub_sets = module_cut_sets_[node->index()];
    if (sub_sets.empty()) {
      std::vector<int> init_path;
      Zbdd::GenerateCutSets(modules_.find(node->index())->second,
                            &init_path, &sub_sets);
    }
    assert(!sub_sets.empty());
    for (const auto& sub_set : sub_sets) {
      std::vector<int> expanded_path(*path);
      expanded_path.insert(expanded_path.end(), sub_set.begin(), sub_set.end());
      Zbdd::GenerateCutSets(node->high(), &expanded_path, cut_sets);
    }
  } else {
    path->push_back(node->index());
    Zbdd::GenerateCutSets(node->high(), path, cut_sets);
  }
}

}  // namespace scram
