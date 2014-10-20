/// @file uncertainty_analysis.h
/// Provides functionality for uncertainty analysis with Monte Carlo method.
#ifndef SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
#define SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <event.h>

class UncertaintyAnalysisTest;

typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;
typedef boost::shared_ptr<scram::BasicEvent> BasicEventPtr;

namespace scram {

class Reporter;

class UncertaintyAnalysis {
friend class ::UncertaintyAnalysisTest;
friend class Reporter;

 public:
  /// The main constructor of Uncertainty Analysis.
  /// @param[in] nsums The number of sums in the probability series.
  /// @param[in] cut_off The cut-off probability for cut sets.
  /// @param[in] num_tirals The number of trials to perform.
  /// @throws InvalidArgument if any of the parameters is invalid.
  explicit UncertaintyAnalysis(int nsums = 7, double cut_off = 1e-8,
                               int num_trials = 1e3);

  /// Set the databases of primary events with probabilities.
  /// Resets the main primary events database and clears the
  /// previous information. This information is the main source for
  /// calculations.
  /// Updates internal indexes for events.
  void UpdateDatabase(const boost::unordered_map<std::string, PrimaryEventPtr>&
                      primary_events);

  /// Performs quantitative analysis on minimal cut sets containing primary
  /// events provided in the databases.
  /// @param[in] min_cut_sets Minimal cut sets with string ids of events.
  ///                         Negative event is indicated by "'not' + id"
  void Analyze(const std::set< std::set<std::string> >& min_cut_sets);

  /// @returns Mean of the final distribution.
  inline double mean() const { return mean_; }

  /// @returns Standard deviation of the final distribution.
  inline double sigma() const { return sigma_; }

  /// @returns The distribution histogram.
  std::vector<int> distribution() const { return distribution_; }

 private:
  /// Assigns an index to each primary event, and then populates with this
  /// indices new databases and primary to integer converting maps.
  /// The previous data are lost.
  /// These indices will be used for future analysis.
  void AssignIndices();

  /// Populates databases of minimal cut sets with indices of the events.
  /// @param[in] min_cut_sets Minimal cut sets with event ids.
  void IndexMcs(const std::set<std::set<std::string> >& min_cut_sets);

  /// Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  /// @param[in] el A set of indices of primary events.
  /// @param[in] set Sets of indices of primary events.
  /// @param[out] combo_set A final set resulting from joining el and sets.
  /// @note O_avg(N*M*logM) where N is the size of the set, and M is the
  /// average size of the elements.
  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >* combo_set);

  /// Generates positive and negative terms of probability equation expansion.
  /// @param[in] sign The sign of the series. Odd int is '+', even int is '-'.
  /// @param[in] nsums The number of sums in the series.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  void ProbOr(int sign, int nsums, std::set< std::set<int> >* min_cut_sets);

  /// Performs Monte Carlo Simulation by sampling the probability distributions
  /// and providing the final sampled values of the final probability.
  void Sample();

  /// Calculates a probability of a minimal cut set, whose members are in AND
  /// relationship with each other. This function assumes independence of each
  /// member. This functionality is provided for the rare event and MCUB,
  /// so the method is not as optimized as the others.
  /// @param[in] min_cut_set A set of indices of primary events.
  /// @returns The total probability.
  /// @note O_avg(N) where N is the size of the passed set.
  double ProbAnd(const std::set<int>& min_cut_set);

  /// Calculates statistical values from the final distribution.
  void CalculateStatistics();

  std::vector< std::set<int> > pos_terms_;  ///< Plus terms of the equation.
  std::vector< std::set<int> > neg_terms_;  ///< Minus terms of the equation.
  std::vector<double> sampled_results_;  ///< Storage for sampled values.

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  std::vector<PrimaryEventPtr> int_to_primary_;  ///< Indices to primary events.
  /// Indices of primary events.
  boost::unordered_map<std::string, int> primary_to_int_;
  /// Indices of house events with state true.
  std::set<int> true_house_events_;
  /// Indices of house events with state false.
  std::set<int> false_house_events_;

  /// Basic events.
  std::vector<BasicEventPtr> basic_events_;

  /// This vector holds sampled probabilities of events.
  std::vector<double> iprobs_;

  /// Minimal cut sets passed for analysis.
  std::set< std::set<std::string> > min_cut_sets_;

  std::vector< std::set<int> > imcs_;  ///< Min cut sets with indices of events.

  /// Number of sums in series expansion for probability calculations.
  int nsums_;

  int num_trials_;  ///< The number of trials to perform.

  double cut_off_;  ///< The cut-off probability for minimal cut sets.

  double p_time_;  ///< Time for probability calculations.

  double mean_;  ///< The mean of the final distribution.
  double sigma_;  ///< The standard deviation of the final distribution.
  std::vector<int> distribution_;  ///< The histogram of the distribution.
};

}  // namespace scram

#endif  // SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
