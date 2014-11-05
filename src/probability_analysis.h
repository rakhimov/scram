/// @file probability_analysis.h
/// Contains funcitonality to do numerical analysis of probabilities and
/// importances.
#ifndef SCRAM_SRC_PROBABILITY_ANALYSIS_H_
#define SCRAM_SRC_PROBABILITY_ANALYSIS_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/flat_set.hpp>

#include <event.h>

class ProbabilityAnalysisTest;
class PerformanceTest;

namespace scram {

class Reporter;

/// @class ProbabilityAnalysis
/// Main quantitative analysis.
class ProbabilityAnalysis {
  friend class ::ProbabilityAnalysisTest;
  friend class ::PerformanceTest;
  friend class Reporter;

 public:
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

  /// The main constructor of Probability Analysis.
  /// @param[in] approx The kind of approximation for probability calculations.
  /// @param[in] nsums The number of sums in the probability series.
  /// @param[in] cut_off The cut-off probability for cut sets.
  /// @throws InvalidArgument if any of the parameters is invalid.
  ProbabilityAnalysis(std::string approx = "no", int nsums = 7,
                      double cut_off = 1e-8);

  virtual ~ProbabilityAnalysis() {}

  /// Set the databases of primary events with probabilities.
  /// Resets the main primary events database and clears the
  /// previous information. This information is the main source for
  /// calculations.
  /// Updates internal indexes for events.
  /// @param[in] basic_events The database of basic event in cut sets.
  void UpdateDatabase(const boost::unordered_map<std::string, BasicEventPtr>&
                      basic_events);

  /// Performs quantitative analysis on minimal cut sets containing primary
  /// events provided in the databases.
  /// @param[in] min_cut_sets Minimal cut sets with string ids of events.
  ///                         Negative event is indicated by "'not' + id"
  virtual void Analyze(const std::set< std::set<std::string> >& min_cut_sets);

  /// @returns The total probability calculated by the analysis.
  /// @note The user should make sure that the analysis is actually done.
  inline double p_total() { return p_total_; }

  /// @returns Map with minimal cut sets and their probabilities.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::set<std::string>, double >& prob_of_min_sets() {
    return prob_of_min_sets_;
  }

  /// @returns Map with primary events and their importance values.
  ///          The associated vector contains DIF, MIF, CIF, RRW, RAW in order.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::string, std::vector<double> >& importance() {
    return importance_;
  }

  /// @returns Container for minimal cut sets ordered by their probabilities.
  /// @todo This should be post-processing.
  inline const std::multimap< double, std::set<std::string> >
      ordered_min_sets() {
    return ordered_min_sets_;
  }

  /// @returns Container for primary events ordered by their contribution.
  /// @todo This should be post-processing.
  inline const std::multimap< double, std::string > ordered_primaries() {
    return ordered_primaries_;
  }

  /// @returns Warnings generated upon analysis.
  inline const std::string warnings() {
    return warnings_;
  }

 protected:
  /// Assigns an index to each primary event, and then populates with this
  /// indices new databases and primary to integer converting maps.
  /// The previous data are lost.
  /// These indices will be used for future analysis.
  void AssignIndices();

  /// Populates databases of minimal cut sets with indices of the events.
  /// This traversal detects if cut sets contain complement events and
  /// turns non-coherent analysis.
  /// @param[in] min_cut_sets Minimal cut sets with event ids.
  void IndexMcs(const std::set<std::set<std::string> >& min_cut_sets);

  /// Calculates probabilities using the minimal cut set upper bound (MCUB)
  /// approximation.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  double ProbMcub(
      const std::vector< boost::container::flat_set<int> >& min_cut_sets);

  /// Generates positive and negative terms of probability equation expansion
  /// a set of minimal cut sets, which are in OR
  /// relationship with each other. This function is a brute force probability
  /// calculation without approximations.
  /// @param[in] sign The sign of the series. Negative or positive number.
  /// @param[in] nsums The number of sums in the series.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  /// @note This function drastically modifies min_cut_sets by deleting
  /// sets inside it. This is for better performance.
  ///
  /// @note O_avg(M*logM*N*2^N) where N is the number of sets, and M is
  /// the average size of the sets.
  void ProbOr(int sign, int nsums,
              std::set< boost::container::flat_set<int> >* min_cut_sets);

  /// Calculates a probability of a minimal cut set, whose members are in AND
  /// relationship with each other. This function assumes independence of each
  /// member.
  /// @param[in] min_cut_set A flat set of indices of primary events.
  /// @returns The total probability.
  /// @note O_avg(N) where N is the size of the passed set.
  double ProbAnd(const boost::container::flat_set<int>& min_cut_set);

  /// Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  /// @param[in] el A set of indices of primary events.
  /// @param[in] set Sets of indices of primary events.
  /// @param[out] combo_set A final set resulting from joining el and sets.
  /// @note O_avg(N*M*logM) where N is the size of the set, and M is the
  /// average size of the elements.
  void CombineElAndSet(
      const boost::container::flat_set<int>& el,
      const std::set< boost::container::flat_set<int> >& set,
      std::set< boost::container::flat_set<int> >* combo_set);

  /// Calculates total probability from the generated probability equation.
  double CalculateTotalProbability();

  /// Importance analysis of basic events that are in minimal cut sets.
  /// @todo This function must aware of approximations.
  void PerformImportanceAnalysis();

  /// Approximations for probability calculations.
  std::string approx_;

  /// Number of sums in series expansion for probability calculations.
  int nsums_;

  /// Cut-off probability for minimal cut sets.
  double cut_off_;

  /// Container for basic events.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  std::vector<BasicEventPtr> int_to_basic_;  ///< Indices to basic events.
  /// Indices of basic events.
  boost::unordered_map<std::string, int> basic_to_int_;
  std::vector<double> iprobs_;  ///< Holds probabilities of primary events.

  /// Minimal cut sets passed for analysis.
  std::set< std::set<std::string> > min_cut_sets_;

  /// Min cut sets with indices of events.
  std::vector< boost::container::flat_set<int> > imcs_;
  /// Indices min cut sets to strings min cut sets mapping.
  /// The same position as in imcs_ container is assumed.
  std::vector< std::set<std::string> > imcs_to_smcs_;
  /// Container for basic event indices that are in minimial cut sets.
  std::set<int> mcs_basic_events_;

  /// Total probability of the top event.
  double p_total_;

  /// Container for minimal cut sets and their respective probabilities.
  std::map< std::set<std::string>, double > prob_of_min_sets_;

  /// Container for minimal cut sets ordered by their probabilities.
  std::multimap< double, std::set<std::string> > ordered_min_sets_;

  /// Container for basic event importance types.
  /// The order is DIF, MIF, CIF, RRW, RAW.
  std::map< std::string, std::vector<double> > importance_;

  /// Container for primary events ordered by their contribution of
  /// Fussel-Vesely.
  /// @todo This should a post processing in reporting.
  std::multimap< double, std::string > ordered_primaries_;

  /// Register warnings.
  std::string warnings_;

  /// The number of minimal cut sets with higher than cut-off probability.
  int num_prob_mcs_;

  bool coherent_;  ///< Indication of coherent optimized analysis.

  double p_time_;  ///< Time for probability calculations.
  double imp_time_;  ///< Time for importance calculations.

  /// Positive terms of the probability equation.
  std::vector< boost::container::flat_set<int> > pos_terms_;
  /// Negative terms of the probability equation.
  std::vector< boost::container::flat_set<int> > neg_terms_;
};

}  // namespace scram

#endif  // SCRAM_SRC_PROBABILITY_ANALYSIS_H_
