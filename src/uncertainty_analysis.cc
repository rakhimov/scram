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

#include <boost/algorithm/string.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/density.hpp>

#include "error.h"
#include "logger.h"

namespace scram {

UncertaintyAnalysis::UncertaintyAnalysis(int num_sums, double cut_off,
                                         int num_trials)
    : ProbabilityAnalysis::ProbabilityAnalysis("no", num_sums, cut_off),
      mean_(-1),
      sigma_(-1),
      analysis_time_(-1) {
  if (num_trials < 1) {
    std::string msg = "The number of trials for uncertainty analysis cannot"
                      " be fewer than 1.";
    throw InvalidArgument(msg);
  }
  num_trials_ = num_trials;
}

void UncertaintyAnalysis::UpdateDatabase(
    const std::unordered_map<std::string, BasicEventPtr>& basic_events) {
  ProbabilityAnalysis::UpdateDatabase(basic_events);
}

void UncertaintyAnalysis::Analyze(
    const std::set< std::set<std::string> >& min_cut_sets) {
  min_cut_sets_ = min_cut_sets;

  // Special case of unity with empty sets.
  if (min_cut_sets_.size() == 1 && min_cut_sets_.begin()->empty()) {
    warnings_ += "Uncertainty for UNITY case.";
    mean_ = 1;
    sigma_ = 0;
    confidence_interval_ = std::make_pair(1, 1);
    distribution_.push_back(std::make_pair(1, 1));
    return;
  }

  ProbabilityAnalysis::IndexMcs(min_cut_sets_);

  using boost::container::flat_set;
  std::set< flat_set<int> > iset;

  std::vector< flat_set<int> >::const_iterator it_min;
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    if (ProbabilityAnalysis::ProbAnd(*it_min) > cut_off_) {
      iset.insert(*it_min);
    }
  }

  CLOCK(analysis_time);
  // Maximum number of sums in the series.
  if (num_sums_ > iset.size()) num_sums_ = iset.size();
  // Generate the equation.
  ProbabilityAnalysis::ProbOr(1, num_sums_, &iset);
  // Sample probabilities and generate data.
  UncertaintyAnalysis::Sample();

  // Perform statistical analysis.
  UncertaintyAnalysis::CalculateStatistics();

  analysis_time_ = DUR(analysis_time);
}

void UncertaintyAnalysis::Sample() {
  using boost::container::flat_set;
  // Detect constant basic events.
  std::vector<int> basic_events;
  UncertaintyAnalysis::FilterUncertainEvents(&basic_events);
  for (int i = 0; i < num_trials_; ++i) {
    // Reset distributions.
    std::vector<int>::iterator it_b;
    // The first element is dummy.
    for (it_b = basic_events.begin(); it_b != basic_events.end(); ++it_b) {
      int_to_basic_[*it_b]->Reset();
    }
    // Sample all basic events with distributions.
    for (it_b = basic_events.begin(); it_b != basic_events.end(); ++it_b) {
      double prob = int_to_basic_[*it_b]->SampleProbability();
      assert(prob >= 0 && prob <= 1);
      iprobs_[*it_b] = prob;
    }
    double pos = 0;
    std::vector< flat_set<int> >::iterator it_s;
    int j = 0;  // Position of the terms.
    for (it_s = pos_terms_.begin(); it_s != pos_terms_.end(); ++it_s) {
      if (it_s->empty()) {
        pos += pos_const_[j];
      } else {
        pos += ProbabilityAnalysis::ProbAnd(*it_s) * pos_const_[j];
      }
      ++j;
    }
    double neg = 0;
    j = 0;
    for (it_s = neg_terms_.begin(); it_s != neg_terms_.end(); ++it_s) {
      if (it_s->empty()) {
        neg += neg_const_[j];
      } else {
        neg += ProbabilityAnalysis::ProbAnd(*it_s) * neg_const_[j];
      }
      ++j;
    }
    sampled_results_.push_back(pos - neg);
  }
}

void UncertaintyAnalysis::FilterUncertainEvents(
    std::vector<int>* basic_events) {
  using boost::container::flat_set;
  std::set<int> const_events;
  std::set<int>::const_iterator it;
  for (it = mcs_basic_events_.begin(); it != mcs_basic_events_.end(); ++it) {
    if (int_to_basic_[*it]->IsConstant()) {
      const_events.insert(const_events.end(), *it);
    } else {
      basic_events->push_back(*it);
    }
  }
  // Pre-calculate for constant events and remove them from sets.
  std::vector< flat_set<int> >::iterator it_set;
  for (it_set = pos_terms_.begin(); it_set != pos_terms_.end(); ++it_set) {
    double const_prob = 1;  // 1 is for multiplication.
    flat_set<int>::iterator it_f;
    for (it_f = it_set->begin(); it_f != it_set->end();) {
      if (const_events.count(std::abs(*it_f))) {
        const_prob *= *it_f > 0 ? iprobs_[*it_f] : 1 - iprobs_[-*it_f];
        it_set->erase(*it_f);
        it_f = it_set->begin();
        continue;
      }
      ++it_f;
    }
    pos_const_.push_back(const_prob);
  }

  for (it_set = neg_terms_.begin(); it_set != neg_terms_.end(); ++it_set) {
    double const_prob = 1;  // 1 is for multiplication.
    flat_set<int>::iterator it_f;
    for (it_f = it_set->begin(); it_f != it_set->end();) {
      if (const_events.count(std::abs(*it_f))) {
        const_prob *= *it_f > 0 ? iprobs_[*it_f] : 1 - iprobs_[-*it_f];
        it_set->erase(*it_f);
        it_f = it_set->begin();
        continue;
      }
      ++it_f;
    }
    neg_const_.push_back(const_prob);
  }
}

void UncertaintyAnalysis::CalculateStatistics() {
  using namespace boost;
  using namespace boost::accumulators;
  accumulator_set<double, stats<tag::mean, tag::variance, tag::density> >
      acc(tag::density::num_bins = 20, tag::density::cache_size = num_trials_);
  std::vector<double>::iterator it;
  for (it = sampled_results_.begin(); it != sampled_results_.end(); ++it) {
    acc(*it);
  }
  typedef iterator_range<std::vector<std::pair<double, double> >::iterator >
      histogram_type;
  histogram_type hist = density(acc);
  for (int i = 0; i < hist.size(); i++) {
    distribution_.push_back(hist[i]);
  }
  mean_ = boost::accumulators::mean(acc);
  sigma_ = std::sqrt(variance(acc));
  confidence_interval_.first = mean_ - sigma_ * 1.96 / std::sqrt(num_trials_);
  confidence_interval_.second = mean_ + sigma_ * 1.96 / std::sqrt(num_trials_);
}

}  // namespace scram
