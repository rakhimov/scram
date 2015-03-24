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

class RiskAnalysis;
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
  /// @param[in] risk_an The main risk analysis with all the model data.
  /// @param[in] settings Configured settings for analysis.
  /// @param[in/out] doc An empty document.
  /// @throws LogicError if the document is not empty.
  void SetupReport(const RiskAnalysis* risk_an, const Settings& settings,
                   xmlpp::Document* doc);

  /// Reports orphan primary events as warnings of the top level.
  /// The warning section of the report should not be initialized.
  /// @param[in] orphan_primary_events Container of orphan events.
  /// @param[in/out] doc Preformatted XML document.
  void ReportOrphans(
      const std::set<boost::shared_ptr<PrimaryEvent> >& orphan_primary_events,
      xmlpp::Document* doc);

  /// Reports the results of analysis to a specified output destination.
  /// @param[in] ft_name The original name of a fault tree.
  /// @param[in] fta Fault Tree Analysis with results.
  /// @param[in] prob_analysis ProbabilityAnalysis with results. Null pointer
  ///                          if there is no probability analysis.
  /// @param[in/out] doc Preformatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportFta(
      std::string ft_name,
      const boost::shared_ptr<const FaultTreeAnalysis>& fta,
      const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
      xmlpp::Document* doc);

  /// Reports results of importance analysis in probability analysis.
  /// @param[in] ft_name The original name of a fault tree.
  /// @param[in] prob_analysis ProbabilityAnalysis with results.
  /// @param[in/out] doc Preformatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportImportance(
      std::string ft_name,
      const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
      xmlpp::Document* doc);

  /// Reports the results of uncertainty analysis with minimal cut sets.
  /// @param[in] ft_name The original name of a fault tree.
  /// @param[in] uncert_analysis UncertaintyAnalysis with results.
  /// @param[in/out] doc Preformatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportUncertainty(
      std::string ft_name,
      const boost::shared_ptr<const UncertaintyAnalysis>& uncert_analysis,
      xmlpp::Document* doc);
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
