/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

#include <cstdlib>

#include "event.h"
#include "logger.h"
#include "zbdd.h"

namespace scram {
namespace core {

ImportanceAnalysis::ImportanceAnalysis(const ProbabilityAnalysis* prob_analysis)
    : Analysis(prob_analysis->settings()) {}

void ImportanceAnalysis::Analyze() noexcept {
  CLOCK(imp_time);
  LOG(DEBUG3) << "Calculating importance factors...";
  double p_total = this->p_total();
  const std::vector<const mef::BasicEvent*>& basic_events =
      this->basic_events();

  std::vector<int> occurrences = this->occurrences();
  for (int i = 0; i < basic_events.size(); ++i) {
    if (occurrences[i] == 0)
      continue;
    const mef::BasicEvent& event = *basic_events[i];
    double p_var = event.p();
    ImportanceFactors imp{};
    imp.occurrence = occurrences[i];
    imp.mif = this->CalculateMif(i);
    if (p_total != 0) {
      imp.cif = p_var * imp.mif / p_total;
      imp.raw = 1 + (1 - p_var) * imp.mif / p_total;
      imp.dif = p_var * imp.raw;
      if (p_total != p_var * imp.mif)
        imp.rrw = p_total / (p_total - p_var * imp.mif);
    }
    importance_.push_back({event, imp});
  }
  LOG(DEBUG3) << "Calculated importance factors in " << DUR(imp_time);
  Analysis::AddAnalysisTime(DUR(imp_time));
}

std::vector<int> ImportanceAnalyzerBase::occurrences() noexcept {
  Pdag::IndexMap<int> result(prob_analyzer_->graph()->basic_events().size());
  for (const std::vector<int>& product : prob_analyzer_->products()) {
    for (int index : product)
      result[std::abs(index)]++;
  }
  return result;
}

double ImportanceAnalyzer<Bdd>::CalculateMif(int index) noexcept {
  index += Pdag::kVariableStartIndex;
  const Bdd::VertexPtr& root = bdd_graph_->root().vertex;
  if (root->terminal())
    return 0;
  bool original_mark = Ite::Ref(root).mark();

  int order = bdd_graph_->index_to_order().find(index)->second;
  double mif = CalculateMif(root, order, !original_mark);
  bdd_graph_->ClearMarks(original_mark);
  return mif;
}

double ImportanceAnalyzer<Bdd>::CalculateMif(const Bdd::VertexPtr& vertex,
                                             int order, bool mark) noexcept {
  if (vertex->terminal())
    return 0;
  Ite& ite = Ite::Ref(vertex);
  if (ite.mark() == mark)
    return ite.factor();
  ite.mark(mark);
  if (ite.order() > order) {
    if (!ite.module()) {
      ite.factor(0);
    } else {  /// @todo Detect if the variable is in the module.
      // The assumption is
      // that the order of a module is always larger
      // than the order of its variables.
      double high = RetrieveProbability(ite.high());
      double low = RetrieveProbability(ite.low());
      if (ite.complement_edge())
        low = 1 - low;
      const Bdd::Function& res =
          bdd_graph_->modules().find(ite.index())->second;
      double mif = CalculateMif(res.vertex, order, mark);
      if (res.complement)
        mif = -mif;
      ite.factor((high - low) * mif);
    }
  } else if (ite.order() == order) {
    assert(!ite.module() && "A variable can't be a module.");
    double high = RetrieveProbability(ite.high());
    double low = RetrieveProbability(ite.low());
    if (ite.complement_edge())
      low = 1 - low;
    ite.factor(high - low);
  } else  {
    assert(ite.order() < order);
    double p_var = 0;
    if (ite.module()) {
      const Bdd::Function& res =
          bdd_graph_->modules().find(ite.index())->second;
      p_var = RetrieveProbability(res.vertex);
      if (res.complement)
        p_var = 1 - p_var;
    } else {
      p_var = prob_analyzer()->p_vars()[ite.index()];
    }
    double high = CalculateMif(ite.high(), order, mark);
    double low = CalculateMif(ite.low(), order, mark);
    if (ite.complement_edge())
      low = -low;
    ite.factor(p_var * high + (1 - p_var) * low);
  }
  return ite.factor();
}

double ImportanceAnalyzer<Bdd>::RetrieveProbability(
    const Bdd::VertexPtr& vertex) noexcept {
  if (vertex->terminal())
    return 1;
  return Ite::Ref(vertex).p();
}

}  // namespace core
}  // namespace scram
