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

 private:
  /// Reports for MC results.
  /// param[in] terms Collection of sets to be printed.
  /// param[in] fta Pointer to the Fault Tree Analysis with the events.
  /// param[in] out The output stream.
  void ReportMcTerms(
      const std::vector< std::set<int> >& terms,
      const boost::shared_ptr<const scram::FaultTreeAnalysis>& fta,
      std::ostream& out);
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
