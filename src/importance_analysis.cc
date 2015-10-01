/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

#include "logger.h"
#include "preprocessor.h"

namespace scram {

ImportanceAnalysis::ImportanceAnalysis(const GatePtr& root,
                                       const Settings& settings)
    : ProbabilityAnalysis::ProbabilityAnalysis(root, settings) {}

void ImportanceAnalysis::Analyze(
    const std::set<std::set<std::string>>& min_cut_sets) noexcept {
  assert(top_event_ && "The fault tree is undefined.");
  assert(!bool_graph_ && "Re-running analysis.");
  // Special case of unity with empty sets.
  if (min_cut_sets.size() == 1 && min_cut_sets.begin()->empty()) {
    warnings_ += " Probability is for UNITY case.";
    p_total_ = 1;
    return;
  }
  ProbabilityAnalysis::AssignIndices();
  ProbabilityAnalysis::IndexMcs(min_cut_sets);

  CLOCK(p_time);
  LOG(DEBUG3) << "Calculating probabilities...";
  // Get the total probability.
  if (kSettings_.approximation() == "mcub") {
    /// @todo Detect the conditions.
    warnings_ += " The MCUB approximation may not hold"
                 " if the fault tree is not coherent"
                 " or there are many common events.";
    p_total_ = ProbabilityAnalysis::ProbMcub(imcs_);

  } else if (kSettings_.approximation() == "rare-event") {
    /// @todo Check if a probability of any cut set exceeds 0.1.
    warnings_ += " The rare event approximation may be inaccurate for analysis"
                 " if minimal cut sets' probabilities exceed 0.1.";
    p_total_ = ProbabilityAnalysis::ProbRareEvent(imcs_);
  } else {
    p_total_ = ProbabilityAnalysis::CalculateTotalProbability();
  }

  assert(p_total_ >= 0 && "The total probability is negative.");
  if (p_total_ > 1) {
    warnings_ += " Probability value exceeded 1 and was adjusted to 1.";
    p_total_ = 1;
  }
  LOG(DEBUG3) << "Finished probability calculations in " << DUR(p_time);
  CLOCK(imp_time);
  LOG(DEBUG3) << "Calculating importance factors...";
  ImportanceAnalysis::PerformImportanceAnalysis();
  LOG(DEBUG3) << "Calculated importance factors in " << DUR(imp_time);
  analysis_time_ = DUR(imp_time);
}

void ImportanceAnalysis::PerformImportanceAnalysis() noexcept {
  if (kSettings_.approximation() == "no") {
    ImportanceAnalysis::PerformImportanceAnalysisBdd();
    return;
  }
  // The main data for all the importance types is P(top/event) or
  // P(top/Not event).
  for (int index : mcs_basic_events_) {
    // Calculate P(top/event)
    var_probs_[index] = 1;
    double p_e = 0;
    if (kSettings_.approximation() == "mcub") {
      p_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else {
      assert(kSettings_.approximation() == "rare-event");
      p_e = ProbabilityAnalysis::ProbRareEvent(imcs_);
    }
    assert(p_e >= 0);
    if (p_e > 1) p_e = 1;

    // Calculate P(top/Not event)
    var_probs_[index] = 0;
    double p_not_e = 0;
    if (kSettings_.approximation() == "mcub") {
      p_not_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else {
      assert(kSettings_.approximation() == "rare-event");
      p_not_e = ProbabilityAnalysis::ProbRareEvent(imcs_);
    }
    assert(p_not_e >= 0);
    if (p_not_e > 1) p_not_e = 1;
    // Restore the probability.
    var_probs_[index] = index_to_basic_[index]->p();

    ImportanceFactors imp;
    double p_var = var_probs_[index];
    imp.mif = p_e - p_not_e;  // Birnbaum Marginal importance factor.
    imp.cif = p_var * imp.mif / p_total_;  // Critical factor.
    imp.raw = 1 + (1 - p_var) * imp.mif / p_total_;  // Risk Achievement Worth.
    imp.dif = p_var * imp.raw;  // Diagnosis importance factor.
    imp.rrw = p_total_ / (p_total_ - p_var * imp.mif);  // Risk Reduction Worth.

    importance_.emplace(index_to_basic_[index]->id(), std::move(imp));
  }
}

void ImportanceAnalysis::PerformImportanceAnalysisBdd() noexcept {
  for (int index : mcs_basic_events_) {
    int order = bdd_graph_->index_to_order().find(index)->second;
    ImportanceFactors imp;
    current_mark_ = !current_mark_;
    imp.mif = ImportanceAnalysis::CalculateMif(bdd_graph_->root().vertex, order,
                                               current_mark_);
    if (bdd_graph_->root().complement) imp.mif = -imp.mif;

    double p_var = var_probs_[index];
    imp.cif = p_var * imp.mif / p_total_;
    imp.raw = 1 + (1 - p_var) * imp.mif / p_total_;
    imp.dif = p_var * imp.raw;
    imp.rrw = p_total_ / (p_total_ - p_var * imp.mif);
    importance_.emplace(index_to_basic_[index]->id(), std::move(imp));

    current_mark_ = !current_mark_;
    ImportanceAnalysis::ClearMarks(bdd_graph_->root().vertex, current_mark_);
  }
}

double ImportanceAnalysis::CalculateMif(const VertexPtr& vertex, int order,
                                        bool mark) noexcept {
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
      double high = ImportanceAnalysis::RetrieveProbability(ite->high());
      double low = ImportanceAnalysis::RetrieveProbability(ite->low());
      if (ite->complement_edge()) low = 1 - low;
      const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
      double mif = ImportanceAnalysis::CalculateMif(res.vertex, order, mark);
      if (res.complement) mif = -mif;
      ite->factor((high - low) * mif);
    }
  } else if (ite->order() == order) {
    assert(!ite->module() && "A variable can't be a module.");
    double high = ImportanceAnalysis::RetrieveProbability(ite->high());
    double low = ImportanceAnalysis::RetrieveProbability(ite->low());
    if (ite->complement_edge()) low = 1 - low;
    ite->factor(high - low);
  } else  {
    assert(ite->order() < order);
    double var_prob = 0;
    if (ite->module()) {
      const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
      var_prob = ImportanceAnalysis::RetrieveProbability(res.vertex);
      if (res.complement) var_prob = 1 - var_prob;
    } else {
      var_prob = var_probs_[ite->index()];
    }
    double high = ImportanceAnalysis::CalculateMif(ite->high(), order, mark);
    double low = ImportanceAnalysis::CalculateMif(ite->low(), order, mark);
    if (ite->complement_edge()) low = -low;
    ite->factor(var_prob * high + (1 - var_prob) * low);
  }
  return ite->factor();
}

double ImportanceAnalysis::RetrieveProbability(
    const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 1;
  return Ite::Ptr(vertex)->prob();
}

void ImportanceAnalysis::ClearMarks(const VertexPtr& vertex,
                                    bool mark) noexcept {
  if (vertex->terminal()) return;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return;
  ite->mark(mark);
  if (ite->module()) {
    const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
    ImportanceAnalysis::ClearMarks(res.vertex, mark);
  }
  ImportanceAnalysis::ClearMarks(ite->high(), mark);
  ImportanceAnalysis::ClearMarks(ite->low(), mark);
}

}  // namespace scram
