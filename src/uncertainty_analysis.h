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

namespace scram {

class UncertaintyAnalysis {
friend class ::UncertaintyAnalysisTest;

 public:
  /// The main constructor of Uncertainty Analysis.
  /// @param[in] nsums The number of sums in the probability series.
  /// @throws ValueError if any of the parameters are invalid.
  explicit UncertaintyAnalysis(int nsums = 1e6);

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

 private:
  /// Number of sums in series expansion for probability calculations.
  int nsums_;

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
  void MProbOr(int sign, int nsums, std::set< std::set<int> >* min_cut_sets);

  /// Performs Monte Carlo Simulation by sampling the probability distributions
  /// and providing the final sampled values of the final probability.
  void MSample();

  std::vector< std::set<int> > pos_terms_;  ///< Plus terms of the equation.
  std::vector< std::set<int> > neg_terms_;  ///< Minus terms of the equation.
  std::vector<double> sampled_results_;  ///< Storage for sampled values.

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  std::vector<PrimaryEventPtr> int_to_primary_;  ///< Indices to primary events.
  /// Indices of primary events.
  boost::unordered_map<std::string, int> primary_to_int_;

  /// Minimal cut sets passed for analysis.
  std::set< std::set<std::string> > min_cut_sets_;

  std::vector< std::set<int> > imcs_;  ///< Min cut sets with indices of events.

  double p_time_;  ///< Time for probability calculations.
};

}  // namespace scram

#endif  // SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
