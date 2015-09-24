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

#include "error.h"
#include "logger.h"

namespace scram {

UncertaintyAnalysis::UncertaintyAnalysis(const GatePtr& root,
                                         const Settings& settings)
    : ProbabilityAnalysis::ProbabilityAnalysis(root, settings),
      kSettings_(settings),
      mean_(0),
      sigma_(0),
      error_factor_(1),
      analysis_time_(-1) {}

void UncertaintyAnalysis::UpdateDatabase(
    const std::unordered_map<std::string, BasicEventPtr>& basic_events) {
  ProbabilityAnalysis::UpdateDatabase(basic_events);
}

void UncertaintyAnalysis::Analyze(
    const std::set< std::set<std::string> >& min_cut_sets) noexcept {
  min_cut_sets_ = min_cut_sets;

  // Special case of unity with empty sets.
  if (min_cut_sets_.size() == 1 && min_cut_sets_.begin()->empty()) {
    warnings_ += "Uncertainty for UNITY case.";
    mean_ = 1;
    sigma_ = 0;
    confidence_interval_ = {1, 1};
    distribution_.emplace_back(1, 1);
    quantiles_.emplace_back(1);
    return;
  }

  CLOCK(analysis_time);
  CLOCK(sample_time);
  LOG(DEBUG3) << "Sampling probabilities...";
  // Sample probabilities and generate data.
  UncertaintyAnalysis::Sample();
  LOG(DEBUG3) << "Finished sampling probabilities in " << DUR(sample_time);

  CLOCK(stat_time);
  LOG(DEBUG3) << "Calculating statistics...";
  // Perform statistical analysis.
  UncertaintyAnalysis::CalculateStatistics();
  LOG(DEBUG3) << "Finished calculating statistics in " << DUR(stat_time);

  analysis_time_ = DUR(analysis_time);
}

void UncertaintyAnalysis::Sample() noexcept {
  sampled_results_.clear();
  sampled_results_.reserve(kSettings_.num_trials());

  // Detect constant basic events.
  std::vector<int> basic_events;
  UncertaintyAnalysis::FilterUncertainEvents(&basic_events);
  for (int i = 0; i < kSettings_.num_trials(); ++i) {
    // Reset distributions.
    for (int index : basic_events) {
      index_to_basic_[index]->Reset();
    }
    // Sample all basic events with distributions.
    for (int index : basic_events) {
      double prob = index_to_basic_[index]->SampleProbability();
      assert(prob >= 0 && prob <= 1);
      var_probs_[index] = prob;
    }
    double result = 0;
    if (kSettings_.approx() == "mcub") {
      result = ProbabilityAnalysis::ProbMcub(imcs_);
    } else if (kSettings_.approx() == "rare-event") {
      result = ProbabilityAnalysis::ProbRareEvent(imcs_);
    } else {
      result = ProbabilityAnalysis::CalculateTotalProbability();
    }
    sampled_results_.push_back(result);
  }
}

void UncertaintyAnalysis::FilterUncertainEvents(
    std::vector<int>* basic_events) noexcept {
  for (const BasicEventPtr& event : ordered_basic_events_) {
    if (!event->IsConstant()) {
      basic_events->push_back(id_to_index_.find(event->id())->second);
    }
  }
}

void UncertaintyAnalysis::CalculateStatistics() noexcept {
  using namespace boost;
  using namespace boost::accumulators;
  using accumulator_q =
      accumulator_set<double, stats<tag::extended_p_square_quantile>>;
  quantiles_.clear();
  int num_quantiles = kSettings_.num_quantiles();
  double delta = 1.0 / num_quantiles;
  for (int i = 0; i < num_quantiles; ++i) {
    quantiles_.push_back(delta * (i + 1));
  }
  accumulator_q acc_q(extended_p_square_probabilities = quantiles_);

  int num_trials = kSettings_.num_trials();
  accumulator_set<double, stats<tag::mean, tag::variance, tag::density> >
      acc(tag::density::num_bins = kSettings_.num_bins(),
          tag::density::cache_size = num_trials);

  std::vector<double>::iterator it;
  for (it = sampled_results_.begin(); it != sampled_results_.end(); ++it) {
    acc(*it);
    acc_q(*it);
  }
  using histogram_type =
      iterator_range<std::vector<std::pair<double, double>>::iterator>;
  histogram_type hist = density(acc);
  for (int i = 1; i < hist.size(); i++) {
    distribution_.push_back(hist[i]);
  }
  mean_ = boost::accumulators::mean(acc);
  double var = variance(acc);
  sigma_ = std::sqrt(var);
  error_factor_ = std::exp(1.96 * sigma_);
  confidence_interval_.first = mean_ - sigma_ * 1.96 / std::sqrt(num_trials);
  confidence_interval_.second = mean_ + sigma_ * 1.96 / std::sqrt(num_trials);

  for (int i = 0; i < num_quantiles; ++i) {
    quantiles_[i] = quantile(acc_q, quantile_probability = quantiles_[i]);
  }
}

}  // namespace scram
