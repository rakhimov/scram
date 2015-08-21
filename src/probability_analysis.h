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

/// @file probability_analysis.h
/// Contains functionality to do numerical analysis
/// of probabilities and importance factors.

#ifndef SCRAM_SRC_PROBABILITY_ANALYSIS_H_
#define SCRAM_SRC_PROBABILITY_ANALYSIS_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/container/flat_set.hpp>

#include "event.h"
#include "settings.h"

namespace scram {

namespace test { class ProbabilityAnalysisTest; }

/// @class ProbabilityAnalysis
/// Main quantitative analysis.
class ProbabilityAnalysis {
  friend class test::ProbabilityAnalysisTest;

 public:
  typedef std::shared_ptr<BasicEvent> BasicEventPtr;

  /// The main constructor of Probability Analysis.
  ///
  /// @param[in] settings Analysis settings for probability calculations.
  explicit ProbabilityAnalysis(const Settings& settings);

  virtual ~ProbabilityAnalysis() {}

  /// Sets the databases of basic events with probabilities.
  /// Resets the main basic event database
  /// and clears the previous information.
  /// This information is the main source
  /// for calculations and internal indexes for basic events.
  ///
  /// @param[in] basic_events The database of basic events in cut sets.
  ///
  /// @note  If not enough information is provided,
  ///        the analysis behavior is undefined.
  void UpdateDatabase(
      const std::unordered_map<std::string, BasicEventPtr>& basic_events);

  /// Performs quantitative analysis on minimal cut sets
  /// containing basic events provided in the databases.
  /// It is assumed that the analysis is called only once.
  ///
  /// @param[in] min_cut_sets Minimal cut sets with string ids of events.
  ///                         Negative event is indicated by "'not' + id"
  ///
  /// @note  Undefined behavior if analysis called two or more times.
  virtual void Analyze(
      const std::set< std::set<std::string> >& min_cut_sets) noexcept;

  /// @returns The total probability calculated by the analysis.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline double p_total() const { return p_total_; }

  /// @returns Map with minimal cut sets and their probabilities.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::set<std::string>, double >&
      prob_of_min_sets() const {
    return prob_of_min_sets_;
  }

  /// @returns Map with basic events and their importance values.
  ///          The associated vector contains
  ///          DIF, MIF, CIF, RRW, RAW in order.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::string, std::vector<double> >&
      importance() const {
    return importance_;
  }

  /// @returns Warnings generated upon analysis.
  inline const std::string warnings() const { return warnings_; }

  /// @returns The container of basic events of supplied for the analysis.
  inline const std::unordered_map<std::string, BasicEventPtr>&
      basic_events() const {
    return basic_events_;
  }

  /// @returns The probability with the rare-event approximation.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline double p_rare() const { return p_rare_; }

  /// @returns Analysis time spent on calculating the total probability.
  inline double prob_analysis_time() const { return p_time_; }

  /// @returns Analysis time spent on calculating the importance factors.
  inline double imp_analysis_time() const { return imp_time_; }

 protected:
  typedef boost::container::flat_set<int> FlatSet;  ///< Faster set.

  /// Assigns an index to each basic event,
  /// and then populates with these indices
  /// new databases and basic-to-integer converting maps.
  /// The previous data are lost.
  /// These indices will be used for future analysis.
  void AssignIndices() noexcept;

  /// Populates databases of minimal cut sets
  /// with indices of the events.
  /// This traversal detects
  /// if cut sets contain complement events
  /// and turns non-coherent analysis.
  ///
  /// @param[in] min_cut_sets Minimal cut sets with event IDs.
  void IndexMcs(const std::set<std::set<std::string> >& min_cut_sets) noexcept;

  /// Calculates probabilities
  /// using the minimal cut set upper bound (MCUB) approximation.
  ///
  /// @param[in] min_cut_sets Sets of indices of basic events.
  ///
  /// @returns The total probability with the MCUB approximation.
  double ProbMcub(const std::vector<FlatSet>& min_cut_sets) noexcept;

  /// Generates positive and negative terms
  /// of probability equation expansion from
  /// a set of cut sets,
  /// which are in OR relationship with each other.
  /// This function is a brute force probability calculation
  /// without approximations.
  ///
  /// @param[in] sign The sign of the series. Negative or positive number.
  /// @param[in] num_sums The number of sums in the series.
  /// @param[in,out] cut_sets Sets of indices of basic events.
  ///
  /// @note This function drastically modifies cut_sets
  ///       by deleting sets inside it.
  ///       This is for better performance.
  /// @note O_avg(M*logM*N*2^N) where N is the number of sets,
  ///       and M is the average size of the sets.
  void ProbOr(int sign, int num_sums, std::set<FlatSet>* cut_sets) noexcept;

  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set A flat set of indices of basic events.
  ///
  /// @returns The total probability of the set.
  ///
  /// @note O_avg(N) where N is the size of the passed set.
  double ProbAnd(const FlatSet& cut_set) noexcept;

  /// Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  ///
  /// @param[in] el A set of indices of basic events.
  /// @param[in] set Sets of indices of basic events.
  /// @param[out] combo_set A final set resulting from joining el and sets.
  ///
  /// @note O_avg(N*M*logM) where N is the size of the set,
  ///       and M is the average size of the elements.
  void CombineElAndSet(const FlatSet& el, const std::set<FlatSet>& set,
                       std::set<FlatSet>* combo_set) noexcept;

  /// Calculates total probability from the generated probability equation.
  double CalculateTotalProbability() noexcept;

  /// Importance analysis of basic events that are in minimal cut sets.
  void PerformImportanceAnalysis() noexcept;


  const Settings kSettings_;  ///< All settings for analysis.
  std::string warnings_;  ///< Register warnings.

  /// Container for basic events.
  std::unordered_map<std::string, BasicEventPtr> basic_events_;

  std::vector<BasicEventPtr> int_to_basic_;  ///< Indices to basic events.
  /// Indices of basic events.
  std::unordered_map<std::string, int> basic_to_int_;
  std::vector<double> iprobs_;  ///< Holds probabilities of basic events.

  /// Minimal cut sets passed for analysis.
  std::set< std::set<std::string> > min_cut_sets_;

  /// Minimal cut sets with indices of events.
  std::vector< boost::container::flat_set<int> > imcs_;
  /// Indices min cut sets to strings min cut sets mapping.
  /// The same position as in imcs_ container is assumed.
  std::vector< std::set<std::string> > imcs_to_smcs_;
  /// Container for basic event indices that are in minimal cut sets.
  std::set<int> mcs_basic_events_;

  double p_total_;  ///< Total probability of the top event.
  double p_rare_;  ///< Total probability applying the rare-event approximation.

  /// Container for minimal cut sets and their respective probabilities.
  std::map< std::set<std::string>, double > prob_of_min_sets_;

  /// Container for basic event importance types.
  /// The order is DIF, MIF, CIF, RRW, RAW.
  std::map< std::string, std::vector<double> > importance_;

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
