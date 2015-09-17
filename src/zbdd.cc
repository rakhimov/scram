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

std::pair<bool, bool> SetNode::ConvertIfTerminal(const ItePtr& ite) noexcept {
  assert(!high_ && !low_ && !high_term_ && !low_term_);
  high_term_ = ite->high_term();
  low_term_ = ite->low_term();
  return {high_term_ != nullptr, low_term_ != nullptr};
}

void SetNode::ReduceApply(const SetNodePtr& node, SetNodePtr* branch,
                          TerminalPtr* branch_term) noexcept {
  if (!node->high_term() || node->high_term()->value()) {
    *branch = node;
    return;
  }
  assert(!node->high_term()->value());  // Empty set.
  if (node->low()) {
    *branch = node->low();
  } else {
    assert(node->low_term());
    *branch_term = node->low_term();
  }
}

Zbdd::Zbdd()
    : kBase_(std::make_shared<Terminal>(true)),
      kEmpty_(std::make_shared<Terminal>(false)),
      set_id_(2) {}

void Zbdd::Analyze(const Bdd* bdd) noexcept {
  SetNodePtr root = Zbdd::ConvertBdd(bdd->root());
  std::vector<int> seed;
  GenerateCutSets(root, &seed);
}

std::shared_ptr<SetNode> Zbdd::ConvertBdd(const ItePtr& ite) noexcept {
  if (!ite) return nullptr;  // Terminal node is handled externally.
  SetNodePtr& zbdd = ites_[ite->id()];
  if (zbdd) return zbdd;
  SetNodePtr high = Zbdd::ConvertBdd(ite->high());
  SetNodePtr low = Zbdd::ConvertBdd(ite->low());

  zbdd = std::make_shared<SetNode>(ite->index(), ite->order());
  std::pair<bool, bool> term = zbdd->ConvertIfTerminal(ite);
  if (!term.first) zbdd->ReduceHigh(high);
  if (!term.second) zbdd->ReduceLow(low);
  if (zbdd->high_term() && !zbdd->high_term()->value()) return zbdd;  // Reduce.
  int id_high = zbdd->IdHigh();
  int id_low = zbdd->IdLow();
  SetNodePtr& in_table = unique_table_[{zbdd->index(), id_high, id_low}];
  if (in_table) {
    zbdd = in_table;
  } else {
    in_table = zbdd;
    zbdd->id(set_id_++);
  }
  return zbdd;
}

void Zbdd::GenerateCutSets(const SetNodePtr& node,
                           std::vector<int>* path) noexcept {
  std::vector<int> seed(*path);
  if (node->low()) {
    Zbdd::GenerateCutSets(node->low(), &seed);
  } else {
    if (!seed.empty()) cut_sets_.emplace_back(std::move(seed));
  }

  path->push_back(node->index());
  if (node->high()) {
    Zbdd::GenerateCutSets(node->high(), path);
  } else {
    cut_sets_.emplace_back(std::move(*path));
  }
}

}  // namespace scram
