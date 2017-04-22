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

///< @todo Use Boost math integration instead.
namespace {  // Integration primitives.

/// Ordered points in ascending X.
using Points = std::vector<std::pair<double, double>>;

/// Integrates over <y, x> points.
double Integrate(const Points& points) {
  assert(points.size() > 1 && "Not enough points for integration.");
  double trapezoid_area = 0;
  for (int i = 1; i < points.size(); ++i) {  // This should get vectorized.
    trapezoid_area += (points[i].first + points[i - 1].first) *
                      (points[i].second - points[i - 1].second);
  }
  trapezoid_area /= 2;  // The division is hoisted out of the loop.
  return trapezoid_area;
}

/// Finds the average y over x with <y, x> points.
double AverageY(const Points& points) {
  double range_x = points.back().second - points.front().second;
  assert(range_x);
  return Integrate(points) / range_x;
}

/// Partitions the f(x) over y axis.
/// Partitioning is normalized.
///
/// @tparam T  The output container type
///            with std::pair<const double, double> value type.
///
/// @param[in] points  The function <y, x> points.
/// @param[out] y_fractions  The ordered buckets to partition Y into.
///
/// @pre The lowest bound for the y_fractions is implicit 0.
template <class T>
void PartitionY(const Points& points, T* y_fractions) {
  for (int i = 1; i < points.size(); ++i) {
    double p_0 = points[i - 1].first;
    double p_1 = points[i].first;
    double t_0 = points[i - 1].second;
    double t_1 = points[i].second;
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
    double b_0 = 0;  // The lower bound of the Y bucket.
    for (std::pair<const double, double>& y_bucket : *y_fractions) {
      double b_1 = y_bucket.first;
      y_bucket.second += fraction(b_0, b_1);
      b_0 = b_1;
    }
  }
  // Normalize the fractions.
  double range_x = points.back().second - points.front().second;
  assert(range_x > 0);
  for (std::pair<const double, double>& y_bucket : *y_fractions)
    y_bucket.second /= range_x;
}

}  // namespace

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
    sil_->pfd_avg = core::AverageY(p_time_);
    core::PartitionY(p_time_, &sil_->pfd_fractions);
    decltype(p_time_) pfh_time;
    pfh_time.reserve(p_time_.size());
    for (const std::pair<double, double>& point : p_time_) {
      pfh_time.emplace_back(point.second ? point.first / point.second : 0,
                            point.second);
    }
    sil_->pfh_avg = core::AverageY(pfh_time);
    core::PartitionY(pfh_time, &sil_->pfh_fractions);
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
         ProbabilityAnalysis::mission_time().value());
  double total_time = ProbabilityAnalysis::mission_time().value();

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
