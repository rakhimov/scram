/// @file reporter.h
/// Reporter of results.
#ifndef SCRAM_SRC_REPORTER_H_
#define SCRAM_SRC_REPORTER_H_

#include <iomanip>
#include <set>
#include <sstream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <libxml++/libxml++.h>

namespace scram {

class RiskAnalysis;
class FaultTreeAnalysis;
class ProbabilityAnalysis;
class UncertaintyAnalysis;
class PrimaryEvent;
class BasicEvent;
class Parameter;
class Settings;

/// @class Reporter
/// This class reports the findings of the analyses.
class Reporter {
 public:
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<Parameter> ParameterPtr;

  /// Sets up XML report document according to a specific standards.
  /// This function populates information about the software, settings,
  /// time, methods, and model. In addition, the function forms the
  /// structure of the overall report for use by other reporting functions.
  /// This function must be called before other reporting functions.
  /// @param[in] risk_an The main risk analysis with all the model data.
  /// @param[in] settings Configured settings for analysis.
  /// @param[in,out] doc An empty document.
  /// @throws LogicError if the document is not empty.
  void SetupReport(const RiskAnalysis* risk_an, const Settings& settings,
                   xmlpp::Document* doc);

  /// Reports orphan primary events as warnings of the top information level.
  /// @param[in] orphan_primary_events Container of orphan events.
  /// @param[in,out] doc Pre-formatted XML document.
  void ReportOrphanPrimaryEvents(
      const std::set<PrimaryEventPtr>& orphan_primary_events,
      xmlpp::Document* doc);

  /// Reports unused parameters as warnings of the top information level.
  /// @param[in] unused_paramters Container of unused parameters.
  /// @param[in,out] doc Pre-formatted XML document.
  void ReportUnusedParameters(const std::set<ParameterPtr>& unused_parameters,
                              xmlpp::Document* doc);

  /// Reports the results of analysis to a specified output destination.
  /// @param[in] ft_name The original name of a fault tree.
  /// @param[in] fta Fault Tree Analysis with results.
  /// @param[in] prob_analysis ProbabilityAnalysis with results. Null pointer
  ///                          if there is no probability analysis.
  /// @param[in,out] doc Pre-formatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportFta(
      std::string ft_name,
      const boost::shared_ptr<const FaultTreeAnalysis>& fta,
      const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
      xmlpp::Document* doc);

  /// Reports results of importance analysis in probability analysis.
  /// @param[in] ft_name The original name of a fault tree.
  /// @param[in] prob_analysis ProbabilityAnalysis with results.
  /// @param[in,out] doc Pre-formatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportImportance(
      std::string ft_name,
      const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
      xmlpp::Document* doc);

  /// Reports the results of uncertainty analysis with minimal cut sets.
  /// @param[in] ft_name The original name of a fault tree.
  /// @param[in] uncert_analysis UncertaintyAnalysis with results.
  /// @param[in,out] doc Pre-formatted XML document.
  /// @note This function must be called only after analysis is done.
  void ReportUncertainty(
      std::string ft_name,
      const boost::shared_ptr<const UncertaintyAnalysis>& uncert_analysis,
      xmlpp::Document* doc);

 private:
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

  /// Detects if a given basic event is a CCF event, and reports it
  /// with a specific formatting.
  /// @param[in] basic_event A basic event to be reported.
  /// @param[in,out] parent A parent element node to have this basic event.
  /// @returns A newly created element node with the event description.
  xmlpp::Element* ReportBasicEvent(const BasicEventPtr& basic_event,
                                   xmlpp::Element* parent);

  /// A generic function to convert numbers to string.
  /// @param[in] num The number to be converted.
  /// @returns Formatted string that represents the number.
  template<class T>
  inline std::string ToString(T num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
  }

  /// A helper function to convert a floating point number to string.
  /// @param[in] num The number to be converted.
  /// @param[in] precision Decimal precision for reporting.
  /// @returns Formatted string that represents the floating point number.
  inline std::string ToString(double num, int precision) {
    std::stringstream ss;
    ss << std::setprecision(precision) << num;
    return ss.str();
  }
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
