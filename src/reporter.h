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
  /// @note This function must be called only after Analyze() function.
  /// param[out] output The output destination.
  /// @throws Error if called before the tree analysis.
  /// @throws IOError if the output file is not accessable.
  void ReportFta(FaultTreeAnalysis* fta,
                 std::map<std::string, std::string>& orig_ids,
                 std::string output);

 private:
};

}  // namespace scram

#endif  // SCRAM_REPORTER_H_
