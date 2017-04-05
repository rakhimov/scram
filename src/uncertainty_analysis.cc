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

/// @file uncertainty_analysis.cc
/// Implements the functionality to run Monte Carlo simulations.

#include "uncertainty_analysis.h"

#include <cmath>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/density.hpp>
#include <boost/accumulators/statistics/extended_p_square_quantile.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include "event.h"
#include "expression.h"
#include "logger.h"

namespace scram {
namespace core {

UncertaintyAnalysis::UncertaintyAnalysis(
    const ProbabilityAnalysis* prob_analysis)
    : Analysis(prob_analysis->settings()),
      mean_(0),
      sigma_(0),
      error_factor_(1) {}

void UncertaintyAnalysis::Analyze() noexcept {
  CLOCK(analysis_time);
  CLOCK(sample_time);
  LOG(DEBUG3) << "Sampling probabilities...";
  // Sample probabilities and generate data.
  std::vector<double> samples = this->Sample();
  LOG(DEBUG3) << "Finished sampling probabilities in " << DUR(sample_time);

  {
    TIMER(DEBUG3, "Calculating statistics");
    CalculateStatistics(samples);  // Perform statistical analysis.
  }

  Analysis::AddAnalysisTime(DUR(analysis_time));
}

std::vector<std::pair<int, mef::Expression&>>
UncertaintyAnalysis::GatherDeviateExpressions(const Pdag* graph) noexcept {
  std::vector<std::pair<int, mef::Expression&>> deviate_expressions;
  int index = Pdag::kVariableStartIndex;
  for (const mef::BasicEvent* event : graph->basic_events()) {
    if (event->expression().IsDeviate())
      deviate_expressions.emplace_back(index, event->expression());
    ++index;
  }
  return deviate_expressions;
}

void UncertaintyAnalysis::SampleExpressions(
    const std::vector<std::pair<int, mef::Expression&>>& deviate_expressions,
    Pdag::IndexMap<double>* p_vars) noexcept {
  // Reset distributions.
  for (const auto& expression : deviate_expressions)
    expression.second.Reset();

  // Sample all expressions with distributions.
  for (const auto& expression : deviate_expressions) {
    double prob = expression.second.Sample();
    (*p_vars)[expression.first] = prob > 1 ? 1 : prob < 0 ? 0 : prob;
  }
}

void UncertaintyAnalysis::CalculateStatistics(
    const std::vector<double>& samples) noexcept {
  using namespace boost;  // NOLINT
  using namespace boost::accumulators;  // NOLINT
  using histogram_type =
      iterator_range<std::vector<std::pair<double, double>>::iterator>;
  quantiles_.clear();
  int num_quantiles = Analysis::settings().num_quantiles();
  double delta = 1.0 / num_quantiles;
  for (int i = 0; i < num_quantiles; ++i) {
    quantiles_.push_back(delta * (i + 1));
  }
  int num_trials = Analysis::settings().num_trials();
  accumulator_set<double, stats<tag::mean, tag::variance, tag::density,
                                tag::extended_p_square_quantile>>
      acc(tag::density::num_bins = Analysis::settings().num_bins(),
          tag::density::cache_size = num_trials,
          extended_p_square_probabilities = quantiles_);
  for (double sample : samples) {
    acc(sample);
  }
  histogram_type hist = density(acc);
  for (int i = 1; i < hist.size(); i++) {
    distribution_.push_back(hist[i]);
  }
  mean_ = boost::accumulators::mean(acc);
  sigma_ = std::sqrt(num_trials * variance(acc) / (num_trials - 1));
  error_factor_ = std::exp(1.96 * sigma_);
  confidence_interval_.first = mean_ - sigma_ * 1.96 / std::sqrt(num_trials);
  confidence_interval_.second = mean_ + sigma_ * 1.96 / std::sqrt(num_trials);

  for (int i = 0; i < num_quantiles; ++i) {
    quantiles_[i] = quantile(acc, quantile_probability = quantiles_[i]);
  }
}

}  // namespace core
}  // namespace scram
