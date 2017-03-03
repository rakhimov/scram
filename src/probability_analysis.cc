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

#include <boost/range/algorithm/find_if.hpp>

#include "event.h"
#include "logger.h"
#include "parameter.h"
#include "settings.h"
#include "zbdd.h"

namespace scram {
namespace core {

ProbabilityAnalysis::ProbabilityAnalysis(const FaultTreeAnalysis* fta,
                                         mef::MissionTime* mission_time)
    : Analysis(fta->settings()),
      p_total_(0),
      mission_time_(mission_time) {}

void ProbabilityAnalysis::Analyze() noexcept {
  CLOCK(p_time);
  LOG(DEBUG3) << "Calculating probabilities...";
  // Get the total probability.
  p_total_ = this->CalculateTotalProbability();
  assert(p_total_ >= 0 && p_total_ <= 1&& "The total probability is invalid.");
  if (p_total_ == 1 &&
      Analysis::settings().approximation() != Approximation::kNone) {
    Analysis::AddWarning("Probability may have been adjusted to 1.");
  }

  p_time_ = this->CalculateProbabilityOverTime();
  if (Analysis::settings().safety_integrity_levels())
    ComputeSil();
  LOG(DEBUG3) << "Finished probability calculations in " << DUR(p_time);
  Analysis::AddAnalysisTime(DUR(p_time));
}

void ProbabilityAnalysis::ComputeSil() noexcept {
  assert(!p_time_.empty() && "The probability over time must be available.");
  assert(!sil_ && "Recomputing the SIL.");
  sil_ = std::make_unique<Sil>();
  if (p_time_.size() == 1) {
    sil_->pfd_avg = p_time_.front().first;
    auto it = boost::find_if(
        sil_->pfd_fractions,
        [this](const std::pair<const double, double>& level) {
          return sil_->pfd_avg <= level.first;
        });
    assert(it != sil_->pfd_fractions.end());
    it->second = 1;
  } else {
    double trapezoid_area = 0;  ///< @todo Use Boost math integration instead.
    for (int i = 1; i < p_time_.size(); ++i) {  // This should get vectorized.
      trapezoid_area += (p_time_[i].first + p_time_[i - 1].first) *
                        (p_time_[i].second - p_time_[i - 1].second);
    }
    trapezoid_area /= 2;  // The division is hoisted out of the loop.
    sil_->pfd_avg =
        trapezoid_area / (p_time_.back().second - p_time_.front().second);

    for (int i = 1; i < p_time_.size(); ++i) {
      double p_0 = p_time_[i - 1].first;
      double p_1 = p_time_[i].first;
      double t_0 = p_time_[i - 1].second;
      double t_1 = p_time_[i].second;
      assert(t_1 > t_0);
      double k = (p_1 - p_0) / (t_1 - t_0);
      if (k < 0) {
        k = -k;
        std::swap(p_1, p_0);
      }
      auto fraction = [&k, &p_1, &p_0, &t_1, &t_0](double b_0, double b_1) {
        if (p_0 <= b_0 && b_1 <= p_1)  // Sub-range.
          return (b_1 - b_0) / k;
        if (b_0 <= p_0 && p_1 <= b_1)  // Super-range.
          return t_1 - t_0;            // Covers the case when k == 0.
        // The cases of partially overlapping intervals.
        if (p_0 <= b_0 && b_0 <= p_1)  // b_1 is outside (>) of the range.
          return (p_1 - b_0) / k;
        if (p_0 <= b_1 && b_1 <= p_1)  // b_0 is outside (<) of the range.
          return (b_1 - p_0) / k;
        return 0.0;  // Ranges do not overlap.
      };
      double b_0 = 0;  // The lower bound of the SIL bucket.
      for (std::pair<const double, double>& sil_bucket : sil_->pfd_fractions) {
        double b_1 = sil_bucket.first;
        sil_bucket.second += fraction(b_0, b_1);
        b_0 = b_1;
      }
    }
    // Normalize the fractions.
    double total_time = p_time_.back().second - p_time_.front().second;
    assert(total_time > 0);
    for (std::pair<const double, double>& sil_bucket : sil_->pfd_fractions)
      sil_bucket.second /= total_time;
  }
}

double CutSetProbabilityCalculator::Calculate(
    const std::vector<int>& cut_set,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  double p_sub_set = 1;  // 1 is for multiplication.
  for (int member : cut_set) {
    assert(member > 0 && "Complements in a cut set.");
    p_sub_set *= p_vars[member];
  }
  return p_sub_set;
}

double RareEventCalculator::Calculate(
    const Zbdd& cut_sets,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  double sum = 0;
  for (const std::vector<int>& cut_set : cut_sets) {
    sum += CutSetProbabilityCalculator::Calculate(cut_set, p_vars);
  }
  return sum > 1 ? 1 : sum;
}

double McubCalculator::Calculate(
    const Zbdd& cut_sets,
    const Pdag::IndexMap<double>& p_vars) noexcept {
  double m = 1;
  for (const std::vector<int>& cut_set : cut_sets) {
    m *= 1 - CutSetProbabilityCalculator::Calculate(cut_set, p_vars);
  }
  return 1 - m;
}

void ProbabilityAnalyzerBase::ExtractVariableProbabilities() {
  p_vars_.reserve(graph_->basic_events().size());
  for (const mef::BasicEvent* event : graph_->basic_events())
    p_vars_.push_back(event->p());
}

std::vector<std::pair<double, double>>
ProbabilityAnalyzerBase::CalculateProbabilityOverTime() noexcept {
  std::vector<std::pair<double, double>> p_time;
  double time_step = Analysis::settings().time_step();
  if (!time_step)
    return p_time;

  assert(Analysis::settings().mission_time() ==
         ProbabilityAnalysis::mission_time().Mean());
  double total_time = ProbabilityAnalysis::mission_time().Mean();

  auto update = [this, &p_time] (double time) {
    mission_time().value(time);
    auto it_p = p_vars_.begin();
    for (const mef::BasicEvent* event : graph_->basic_events())
      *it_p++ = event->p();
    p_time.emplace_back(this->CalculateTotalProbability(p_vars_), time);
  };

  for (double time = 0; time < total_time; time += time_step)
    update(time);
  update(total_time);  // Handle cases when total_time is not divisible by step.
  return p_time;
}

ProbabilityAnalyzer<Bdd>::ProbabilityAnalyzer(FaultTreeAnalyzer<Bdd>* fta,
                                              mef::MissionTime* mission_time)
    : ProbabilityAnalyzerBase(fta, mission_time),
      owner_(false) {
  LOG(DEBUG2) << "Re-using BDD from FaultTreeAnalyzer for ProbabilityAnalyzer";
  bdd_graph_ = fta->algorithm();
  const Bdd::VertexPtr& root = bdd_graph_->root().vertex;
  current_mark_ = root->terminal() ? false : Ite::Ref(root).mark();
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
  Ite& ite = Ite::Ref(vertex);
  if (ite.mark() == mark)
    return ite.p();
  ite.mark(mark);
  double p_var = 0;
  if (ite.module()) {
    const Bdd::Function& res = bdd_graph_->modules().find(ite.index())->second;
    p_var = CalculateProbability(res.vertex, mark, p_vars);
    if (res.complement)
      p_var = 1 - p_var;
  } else {
    p_var = p_vars[ite.index()];
  }
  double high = CalculateProbability(ite.high(), mark, p_vars);
  double low = CalculateProbability(ite.low(), mark, p_vars);
  if (ite.complement_edge())
    low = 1 - low;
  ite.p(p_var * high + (1 - p_var) * low);
  return ite.p();
}

}  // namespace core
}  // namespace scram
