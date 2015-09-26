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

Zbdd::Zbdd()
    : kBase_(std::make_shared<Terminal>(true)),
      kEmpty_(std::make_shared<Terminal>(false)),
      set_id_(2) {}

void Zbdd::Analyze(const Bdd* bdd) noexcept {
  SetNodePtr root =
      SetNode::Ptr(Zbdd::ConvertBdd(bdd->root(), bdd->complement_root()));
  Zbdd::Subsume(root);
  std::vector<int> seed;
  Zbdd::GenerateCutSets(root, &seed);
}

std::shared_ptr<Vertex> Zbdd::ConvertBdd(const VertexPtr& vertex,
                                         bool complement) noexcept {
  if (vertex->terminal()) return complement ? kEmpty_ : kBase_;
  int sign = complement ? -1 : 1;
  SetNodePtr& zbdd = ites_[sign * vertex->id()];
  if (zbdd) return zbdd;
  ItePtr ite = Ite::Ptr(vertex);
  zbdd = std::make_shared<SetNode>(ite->index(), ite->order());
  zbdd->high(Zbdd::ConvertBdd(ite->high(), complement));
  zbdd->low(Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement));
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
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return node;
  node->mark(true);
  node->high(Zbdd::Subsume(node->high(), node->low()));
  Zbdd::Subsume(node->high());
  Zbdd::Subsume(node->low());
  return node;
}

std::shared_ptr<Vertex> Zbdd::Subsume(const VertexPtr& high,
                                      const VertexPtr& low) noexcept {
  if (low->terminal()) {
    if (Terminal::Ptr(low)->value()) {
      return kEmpty_;
    } else {
      return high;
    }
  }
  if (high->terminal()) return high;
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
    existing_node->high(subhigh);
    existing_node->low(sublow);
    existing_node->id(set_id_++);
  }
  computed = existing_node;
  return computed;
}

void Zbdd::GenerateCutSets(const VertexPtr& vertex,
                           std::vector<int>* path) noexcept {
  if (vertex->terminal()) {
    if (Terminal::Ptr(vertex)->value()) {
      cut_sets_.emplace_back(std::move(*path));
    }
    return;  // Don't include 0/NULL sets.
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  std::vector<int> seed(*path);
  Zbdd::GenerateCutSets(node->low(), &seed);

  path->push_back(node->index());
  Zbdd::GenerateCutSets(node->high(), path);
}

}  // namespace scram
