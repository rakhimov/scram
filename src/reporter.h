/// @file reporter.h
/// Reporter of results.
#ifndef SCRAM_SRC_REPORTER_H_
#define SCRAM_SRC_REPORTER_H_

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "fault_tree_analysis.h"
#include "probability_analysis.h"

typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

namespace scram {
/// @class Reporter
/// This class reports the findings of the analyses.
class Reporter {
 public:
  /// Reports orphan primary events.
  /// @param[in] orphan_primary_events Container of orphan events.
  /// @param[out] output The output destination.
  void ReportOrphans(
      const std::set<boost::shared_ptr<scram::PrimaryEvent> >&
          orphan_primary_events,
      std::string output);

  /// Reports the results of analysis to a specified output destination.
  /// @param[in] fta Fault Tree Analysis with results.
  /// @param[out] output The output destination.
  /// @note This function must be called only after analysis is done.
  void ReportFta(const boost::shared_ptr<const scram::FaultTreeAnalysis>& fta,
                 std::string output);

  /// Reports the results of probability analysis with minimal cut sets.
  /// @param[in] prob_analysis ProbabilityAnalysis with results.
  /// @param[out] output The output destination.
  /// @note This function must be called only after analysis is done.
  void ReportProbability(
      const boost::shared_ptr<const scram::ProbabilityAnalysis>& prob_analysis,
      std::string output);

 private:
  /// Produces lines for printing minimal cut sets.
  /// @param[in] min_cut_sets Minimal cut sets to print.
  /// @param[in] primary_events Primary events in the minimal cut sets.
  /// @param[out] lines Lines representing minimal cut sets.
  void McsToPrint(
      const std::set< std::set<std::string> >& min_cut_sets,
      const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events,
      std::map< std::set<std::string>, std::vector<std::string> >* lines);
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
