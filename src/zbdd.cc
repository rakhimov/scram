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
  SetNodePtr root = SetNode::Ptr(Zbdd::ConvertBdd(bdd->root()));
  std::vector<int> seed;
  GenerateCutSets(root, &seed);
}

std::shared_ptr<Vertex> Zbdd::ConvertBdd(const VertexPtr& vertex) noexcept {
  if (vertex->terminal())
    return Terminal::Ptr(vertex)->value() ? kBase_ : kEmpty_;
  SetNodePtr& zbdd = ites_[vertex->id()];
  if (zbdd) return zbdd;
  ItePtr ite = Ite::Ptr(vertex);
  zbdd = std::make_shared<SetNode>(ite->index(), ite->order());
  zbdd->high(Zbdd::ConvertBdd(ite->high()));
  zbdd->low(Zbdd::ConvertBdd(ite->low()));
  if (zbdd->high()->terminal() && !Terminal::Ptr(zbdd->high())->value())
    return zbdd->low();  // Reduce.
  SetNodePtr& in_table =
      unique_table_[{zbdd->index(), zbdd->high()->id(), zbdd->low()->id()}];
  if (in_table) return in_table;
  in_table = zbdd;
  zbdd->id(set_id_++);
  return zbdd;
}

void Zbdd::GenerateCutSets(const VertexPtr& vertex,
                           std::vector<int>* path) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  std::vector<int> seed(*path);
  Zbdd::GenerateCutSets(node->low(), &seed);
  if (!seed.empty()) cut_sets_.emplace_back(std::move(seed));

  path->push_back(node->index());
  Zbdd::GenerateCutSets(node->high(), path);
  cut_sets_.emplace_back(std::move(*path));
}

}  // namespace scram
