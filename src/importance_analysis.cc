/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file importance_analysis.cc
/// Implementations of functions to provide
/// quantitative importance informations.

#include "importance_analysis.h"

#include <algorithm>
#include <unordered_set>

#include "logger.h"

namespace scram {
namespace core {

ImportanceAnalysis::ImportanceAnalysis(const ProbabilityAnalysis* prob_analysis)
    : Analysis(prob_analysis->settings()) {}

void ImportanceAnalysis::Analyze() noexcept {
  CLOCK(imp_time);
  LOG(DEBUG3) << "Calculating importance factors...";
  std::vector<std::pair<int, mef::BasicEventPtr>> target_events =
      this->GatherImportantEvents();
  double p_total = this->p_total();
  for (const auto& event : target_events) {
    double p_var = event.second->p();
    ImportanceFactors imp;
    imp.mif = this->CalculateMif(event.first);
    imp.cif = p_var * imp.mif / p_total;
    imp.raw = 1 + (1 - p_var) * imp.mif / p_total;
    imp.dif = p_var * imp.raw;
    imp.rrw = p_total / (p_total - p_var * imp.mif);
    importance_.emplace(event.second->id(), imp);
    important_events_.emplace_back(event.second, imp);
  }
  LOG(DEBUG3) << "Calculated importance factors in " << DUR(imp_time);
  Analysis::AddAnalysisTime(DUR(imp_time));
}

std::vector<std::pair<int, mef::BasicEventPtr>>
ImportanceAnalysis::GatherImportantEvents(
    const BooleanGraph* graph,
    const std::vector<std::vector<int>>& products) noexcept {
  std::vector<std::pair<int, mef::BasicEventPtr>> important_events;
  std::unordered_set<int> unique_indices;
  for (const auto& product : products) {
    for (int index : product) {
      if (unique_indices.count(std::abs(index))) continue;  // Most likely.
      int pos_index = std::abs(index);
      unique_indices.insert(pos_index);
      important_events.emplace_back(pos_index,
                                    graph->GetBasicEvent(pos_index));
    }
  }
  return important_events;
}

double ImportanceAnalyzer<Bdd>::CalculateMif(int index) noexcept {
  Bdd::VertexPtr root = bdd_graph_->root().vertex;
  if (root->terminal()) return 0;
  bool original_mark = Ite::Ptr(root)->mark();

  int order = bdd_graph_->index_to_order().find(index)->second;
  double mif = ImportanceAnalyzer::CalculateMif(bdd_graph_->root().vertex,
                                                order,
                                                !original_mark);
  bdd_graph_->ClearMarks(original_mark);
  return mif;
}

double ImportanceAnalyzer<Bdd>::CalculateMif(const Bdd::VertexPtr& vertex,
                                             int order, bool mark) noexcept {
  if (vertex->terminal()) return 0;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return ite->factor();
  ite->mark(mark);
  if (ite->order() > order) {
    if (!ite->module()) {
      ite->factor(0);
    } else {  /// @todo Detect if the variable is in the module.
      // The assumption is
      // that the order of a module is always larger
      // than the order of its variables.
      double high = ImportanceAnalyzer::RetrieveProbability(ite->high());
      double low = ImportanceAnalyzer::RetrieveProbability(ite->low());
      if (ite->complement_edge()) low = 1 - low;
      const Bdd::Function& res =
          bdd_graph_->modules().find(ite->index())->second;
      double mif = ImportanceAnalyzer::CalculateMif(res.vertex, order, mark);
      if (res.complement) mif = -mif;
      ite->factor((high - low) * mif);
    }
  } else if (ite->order() == order) {
    assert(!ite->module() && "A variable can't be a module.");
    double high = ImportanceAnalyzer::RetrieveProbability(ite->high());
    double low = ImportanceAnalyzer::RetrieveProbability(ite->low());
    if (ite->complement_edge()) low = 1 - low;
    ite->factor(high - low);
  } else  {
    assert(ite->order() < order);
    double p_var = 0;
    if (ite->module()) {
      const Bdd::Function& res =
          bdd_graph_->modules().find(ite->index())->second;
      p_var = ImportanceAnalyzer::RetrieveProbability(res.vertex);
      if (res.complement) p_var = 1 - p_var;
    } else {
      p_var = ImportanceAnalyzerBase::prob_analyzer()->p_vars()[ite->index()];
    }
    double high = ImportanceAnalyzer::CalculateMif(ite->high(), order, mark);
    double low = ImportanceAnalyzer::CalculateMif(ite->low(), order, mark);
    if (ite->complement_edge()) low = -low;
    ite->factor(p_var * high + (1 - p_var) * low);
  }
  return ite->factor();
}

double ImportanceAnalyzer<Bdd>::RetrieveProbability(
    const Bdd::VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 1;
  return Ite::Ptr(vertex)->p();
}

}  // namespace core
}  // namespace scram
