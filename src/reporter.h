/// @file reporter.h
/// Reporter of results.
#ifndef SCRAM_REPORTER_H_
#define SCRAM_REPORTER_H_

#include <map>

#include "fault_tree_analysis.h"

namespace scram {
/// @class Reporter
/// Reporter of findings of analysis.
class Reporter {
 public:
  Reporter();

  /// Reports the results of analysis to a specified output destination.
  /// @param[in] fta Fault Tree Analysis with results.
  /// @param[in] orig_ids Map of original names for better reporting.
  /// @param[out] output The output destination.
  /// @throws IOError if the output file is not accessable.
  /// @note This function must be called only after analysis is done.
  void ReportFta(const FaultTreeAnalysis* fta,
                 const std::map<std::string, std::string>& orig_ids,
                 std::string output);
};

}  // namespace scram

#endif  // SCRAM_REPORTER_H_
