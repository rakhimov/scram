/// @file reporter.h
/// Reporter of results.
#ifndef SCRAM_SRC_REPORTER_H_
#define SCRAM_SRC_REPORTER_H_

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <libxml++/libxml++.h>

namespace scram {

class FaultTreeAnalysis;
class ProbabilityAnalysis;
class UncertaintyAnalysis;
class PrimaryEvent;
class BasicEvent;
class Settings;

/// @class Reporter
/// This class reports the findings of the analyses.
class Reporter {
 public:
  /// Sets up XML report document according to a specific standards.
  /// This function populates information about the software, settings,
  /// time, methods, and model. In addition, the function forms the
  /// structure of the overall report for use by other reporting functions.
  /// This function must be called before other reporting functions.
  /// @param[in] settings Configured settings for analysis.
  /// @param[in/out] doc An empty document.
  /// @throws LogicError if the document is not empty.
  void SetupReport(const Settings& settings, xmlpp::Document* doc);

  /// Reports orphan primary events.
  /// @param[in] orphan_primary_events Container of orphan events.
  /// @param[out] out Output stream.
  void ReportOrphans(
      const std::set<boost::shared_ptr<PrimaryEvent> >& orphan_primary_events,
      std::ostream& out);

  /// Reports the results of analysis to a specified output destination.
  /// @param[in] fta Fault Tree Analysis with results.
  /// @param[in/out] doc Preformatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportFta(const boost::shared_ptr<const FaultTreeAnalysis>& fta,
                 xmlpp::Document* doc);

  /// Reports the results of probability analysis with minimal cut sets.
  /// @param[in] prob_analysis ProbabilityAnalysis with results.
  /// @param[out] out Output stream.
  /// @note This function must be called only after analysis is done.
  void ReportProbability(
      const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
      std::ostream& out);

  /// Reports the results of uncertainty analysis with minimal cut sets.
  /// @param[in] uncert_analysis UncertaintyAnalysis with results.
  /// @param[in/out] doc Preformatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportUncertainty(
      const boost::shared_ptr<const UncertaintyAnalysis>& uncert_analysis,
      xmlpp::Document* doc);

 private:
  /// Reports results of importance analysis in probability analysis.
  /// @param[in] prob_analysis ProbabilityAnalysis with results.
  /// Reports as "Primary Event Analysis".
  /// @param[out] out Output stream.
  void ReportImportance(
      const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
      std::ostream& out);
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
