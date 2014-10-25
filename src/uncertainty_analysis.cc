/// @file uncertainty_analysis.cc
/// Implements the functionality to run Monte Carlo uncertainty analysis.
#include "uncertainty_analysis.h"

#include <ctime>
#include <cmath>

#include <boost/algorithm/string.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/density.hpp>

#include "error.h"

namespace scram {

UncertaintyAnalysis::UncertaintyAnalysis(int nsums, double cut_off,
                                         int num_trials)
    : ProbabilityAnalysis::ProbabilityAnalysis("no", nsums, cut_off),
      mean_(-1),
      sigma_(-1),
      p_time_(-1) {
  if (num_trials < 1) {
    std::string msg = "The number of trials for uncertainty analysis cannot"
                      " be fewer than 1.";
    throw InvalidArgument(msg);
  }
  num_trials_ = num_trials;
}

void UncertaintyAnalysis::UpdateDatabase(
    const boost::unordered_map<std::string, BasicEventPtr>& basic_events) {
  ProbabilityAnalysis::UpdateDatabase(basic_events);
}

void UncertaintyAnalysis::Analyze(
    const std::set< std::set<std::string> >& min_cut_sets) {
  min_cut_sets_ = min_cut_sets;

  ProbabilityAnalysis::IndexMcs(min_cut_sets_);

  using boost::container::flat_set;
  std::set< flat_set<int> > iset;

  std::vector< flat_set<int> >::const_iterator it_min;
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    if (ProbabilityAnalysis::ProbAnd(*it_min) > cut_off_) {
      iset.insert(*it_min);
    }
  }

  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();

  // Maximum number of sums in the series.
  if (nsums_ > iset.size()) nsums_ = iset.size();
  // Generate the equation.
  ProbabilityAnalysis::ProbOr(1, nsums_, &iset);
  // Sample probabilities and generate data.
  UncertaintyAnalysis::Sample();

  // Perform statistical analysis.
  UncertaintyAnalysis::CalculateStatistics();

  // Duration of the calculations.
  p_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
}

void UncertaintyAnalysis::Sample() {
  using boost::container::flat_set;
  std::vector<int> basic_events(mcs_basic_events_.begin(),
                                mcs_basic_events_.end());
  for (int i = 0; i < num_trials_; ++i) {
    // Reset distributions.
    std::vector<int>::iterator it_b;
    // The first element is dummy.
    for (it_b = basic_events.begin(); it_b != basic_events.end(); ++it_b) {
      int_to_basic_[*it_b]->Reset();
    }
    // Sample all basic events.
    /// @todo Do not sample constant values(i.e. events without distributions.)
    for (it_b = basic_events.begin(); it_b != basic_events.end(); ++it_b) {
      double prob = int_to_basic_[*it_b]->SampleProbability();
      assert(prob >= 0 && prob <= 1);
      iprobs_[*it_b] = prob;
    }
    double pos = 0;
    std::vector< flat_set<int> >::iterator it_s;
    for (it_s = pos_terms_.begin(); it_s != pos_terms_.end(); ++it_s) {
      pos += ProbabilityAnalysis::ProbAnd(*it_s);
    }
    double neg = 0;
    for (it_s = neg_terms_.begin(); it_s != neg_terms_.end(); ++it_s) {
      neg += ProbabilityAnalysis::ProbAnd(*it_s);
    }
    sampled_results_.push_back(pos - neg);
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
