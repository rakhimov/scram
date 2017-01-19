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

/// @file probability_analysis.cc
/// Implementations of functions to provide
/// probability analysis.

#include "probability_analysis.h"

#include "event.h"
#include "logger.h"
#include "settings.h"

namespace scram {
namespace core {

ProbabilityAnalysis::ProbabilityAnalysis(const FaultTreeAnalysis* fta)
    : Analysis(fta->settings()),
      p_total_(0) {}

void ProbabilityAnalysis::Analyze() noexcept {
  CLOCK(p_time);
  LOG(DEBUG3) << "Calculating probabilities...";
  // Get the total probability.
  p_total_ = this->CalculateTotalProbability();
  assert(p_total_ >= 0 && "The total probability is negative.");
  if (p_total_ > 1) {
    Analysis::AddWarning("Probability value exceeded 1 and was adjusted to 1.");
    p_total_ = 1;
  }
  LOG(DEBUG3) << "Finished probability calculations in " << DUR(p_time);
  Analysis::AddAnalysisTime(DUR(p_time));
}

double CutSetProbabilityCalculator::Calculate(
    const CutSet& cut_set,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  if (cut_set.empty())
    return 0;
  double p_sub_set = 1;  // 1 is for multiplication.
  for (int member : cut_set) {
    assert(member > 0 && "Complements in a cut set.");
    p_sub_set *= p_vars[member];
  }
  return p_sub_set;
}

double RareEventCalculator::Calculate(
    const std::vector<CutSet>& cut_sets,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  if (CutSetProbabilityCalculator::CheckUnity(cut_sets))
    return 1;
  double sum = 0;
  for (const auto& cut_set : cut_sets) {
    assert(!cut_set.empty() && "Detected an empty cut set.");
    sum += CutSetProbabilityCalculator::Calculate(cut_set, p_vars);
  }
  return sum > 1 ? 1 : sum;
}

double McubCalculator::Calculate(
    const std::vector<CutSet>& cut_sets,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  if (CutSetProbabilityCalculator::CheckUnity(cut_sets))
    return 1;
  double m = 1;
  for (const auto& cut_set : cut_sets) {
    assert(!cut_set.empty() && "Detected an empty cut set.");
    m *= 1 - CutSetProbabilityCalculator::Calculate(cut_set, p_vars);
  }
  return 1 - m;
}

void ProbabilityAnalyzerBase::ExtractVariableProbabilities() {
  p_vars_.reserve(graph_->basic_events().size());
  for (const mef::BasicEvent* event : graph_->basic_events())
    p_vars_.push_back(event->p());
}

ProbabilityAnalyzer<Bdd>::ProbabilityAnalyzer(FaultTreeAnalyzer<Bdd>* fta)
    : ProbabilityAnalyzerBase(fta),
      owner_(false) {
  LOG(DEBUG2) << "Re-using BDD from FaultTreeAnalyzer for ProbabilityAnalyzer";
  bdd_graph_ = fta->algorithm();
  Bdd::VertexPtr root = bdd_graph_->root().vertex;
  current_mark_ = root->terminal() ? false : Ite::Ptr(root)->mark();
}

ProbabilityAnalyzer<Bdd>::~ProbabilityAnalyzer() noexcept {
  if (owner_)
    delete bdd_graph_;
}

double ProbabilityAnalyzer<Bdd>::CalculateTotalProbability(
    const Pdag::IndexMap<double>& p_vars) noexcept {
  CLOCK(calc_time);  // BDD based calculation time.
  LOG(DEBUG4) << "Calculating probability with BDD...";
  current_mark_ = !current_mark_;
  double prob =
      CalculateProbability(bdd_graph_->root().vertex, current_mark_, p_vars);
  if (bdd_graph_->root().complement)
    prob = 1 - prob;
  LOG(DEBUG4) << "Calculated probability " << prob << " in " << DUR(calc_time);
  return prob;
}

void ProbabilityAnalyzer<Bdd>::CreateBdd(
    const FaultTreeAnalysis& fta) noexcept {
  CLOCK(total_time);

  CLOCK(ft_creation);
  Pdag graph(fta.top_event(), Analysis::settings().ccf_analysis());
  LOG(DEBUG2) << "PDAG is created in " << DUR(ft_creation);

  CLOCK(prep_time);  // Overall preprocessing time.
  LOG(DEBUG2) << "Preprocessing...";
  CustomPreprocessor<Bdd>{&graph}();
  LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);

  CLOCK(bdd_time);  // BDD based calculation time.
  LOG(DEBUG2) << "Creating BDD for Probability Analysis...";
  bdd_graph_ = new Bdd(&graph, Analysis::settings());
  LOG(DEBUG2) << "BDD is created in " << DUR(bdd_time);

  Analysis::AddAnalysisTime(DUR(total_time));
}

double ProbabilityAnalyzer<Bdd>::CalculateProbability(
    const Bdd::VertexPtr& vertex,
    bool mark,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  if (vertex->terminal())
    return 1;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark)
    return ite->p();
  ite->mark(mark);
  double p_var = 0;
  if (ite->module()) {
    const Bdd::Function& res = bdd_graph_->modules().find(ite->index())->second;
    p_var = CalculateProbability(res.vertex, mark, p_vars);
    if (res.complement)
      p_var = 1 - p_var;
  } else {
    p_var = p_vars[ite->index()];
  }
  double high = CalculateProbability(ite->high(), mark, p_vars);
  double low = CalculateProbability(ite->low(), mark, p_vars);
  if (ite->complement_edge())
    low = 1 - low;
  ite->p(p_var * high + (1 - p_var) * low);
  return ite->p();
}

}  // namespace core
}  // namespace scram
