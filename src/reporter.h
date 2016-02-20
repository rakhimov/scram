/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file reporter.h
/// Reporter of results.

#ifndef SCRAM_SRC_REPORTER_H_
#define SCRAM_SRC_REPORTER_H_

#include <memory>
#include <string>
#include <vector>

#include <libxml++/libxml++.h>

namespace scram {

class Model;
class Settings;
class BasicEvent;
class FaultTreeAnalysis;
class ProbabilityAnalysis;
class ImportanceAnalysis;
class UncertaintyAnalysis;
class RiskAnalysis;

/// @class Reporter
/// This class reports the results of the analyses.
class Reporter {
 public:
  /// Reports the results of risk analysis on a model.
  /// The XML report is formed as a single document.
  ///
  /// @param[in] risk_an  Risk analysis with results.
  /// @param[out] out  The report destination stream.
  ///
  /// @pre The output destination is used only by this reporter.
  ///      There is going to be no appending to the stream after the report.
  void Report(const RiskAnalysis& risk_an, std::ostream& out);

 private:
  using BasicEventPtr = std::shared_ptr<const BasicEvent>;  ///< For simplicity.

  /// This function populates information
  /// about the software, settings, time, methods, model, etc.
  ///
  /// @param[in] risk_an  Risk analysis with all the information.
  /// @param[in,out] report  The root element of the document.
  void ReportInformation(const RiskAnalysis& risk_an, xmlpp::Element* report);

  /// Reports software information and relevant run identifiers.
  ///
  /// @param[in,out] information  The XML element to append the results.
  void ReportSoftwareInformation(xmlpp::Element* information);

  /// Reports information about calculated quantities.
  /// The default call reports everything about the analysis
  /// as requested by settings.
  ///
  /// @tparam T  The kind of analysis information to be reported.
  ///
  /// @param[in] settings  The whole analysis settings.
  /// @param[in,out] information  The XML element to append the results.
  template<class T = RiskAnalysis>
  void ReportCalculatedQuantity(const Settings& settings,
                                xmlpp::Element* information);

  /// Reports summary of the model and its constructs.
  ///
  /// @param[in] model  The container of all the analysis constructs.
  /// @param[in,out] information  The XML element to append the results.
  void ReportModelFeatures(const Model& model, xmlpp::Element* information);

  /// Reports performance metrics of all conducted analyses.
  ///
  /// @param[in] risk_an  Container of the analyses.
  /// @param[in,out] information  The XML element to append the results.
  void ReportPerformance(const RiskAnalysis& risk_an,
                         xmlpp::Element* information);

  /// Reports orphan primary events
  /// as warnings of the top information level.
  ///
  /// @param[in] model  Model containing all primary events.
  /// @param[in,out] information  The XML element to append the results.
  void ReportOrphanPrimaryEvents(const Model& model,
                                 xmlpp::Element* information);

  /// Reports unused parameters
  /// as warnings of the top information level.
  ///
  /// @param[in] model  Model containing all parameters.
  /// @param[in,out] information  The XML element to append the results.
  void ReportUnusedParameters(const Model& model, xmlpp::Element* information);

  /// Reports the results of fault tree analysis
  /// to a specified output destination.
  ///
  /// @param[in] ft_name  The original name of a fault tree.
  /// @param[in] fta  Fault Tree Analysis with results.
  /// @param[in] prob_analysis  Probability Analysis with results.
  ///                           Null pointer for no probability analysis.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(std::string ft_name, const FaultTreeAnalysis& fta,
                     const ProbabilityAnalysis* prob_analysis,
                     xmlpp::Element* results);

  /// Reports results of importance analysis.
  ///
  /// @param[in] ft_name  The original name of a fault tree.
  /// @param[in] importance_analysis  Importance analysis with results.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(std::string ft_name,
                     const ImportanceAnalysis& importance_analysis,
                     xmlpp::Element* results);

  /// Reports the results of uncertainty analysis.
  ///
  /// @param[in] ft_name  The original name of a fault tree.
  /// @param[in] uncert_analysis  Uncertainty analysis with results.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(std::string ft_name,
                     const UncertaintyAnalysis& uncert_analysis,
                     xmlpp::Element* results);

  /// Detects if a given basic event is a CCF event,
  /// and reports it with specific formatting.
  ///
  /// @param[in] basic_event  A basic event to be reported.
  /// @param[in,out] parent  A parent element node to have this basic event.
  ///
  /// @returns A newly created element node with the event description.
  xmlpp::Element* ReportBasicEvent(const BasicEventPtr& basic_event,
                                   xmlpp::Element* parent);
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
