/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

#include <cstdio>

#include <string>

#include "event.h"
#include "fault_tree_analysis.h"
#include "importance_analysis.h"
#include "model.h"
#include "probability_analysis.h"
#include "risk_analysis.h"
#include "settings.h"
#include "uncertainty_analysis.h"
#include "xml_stream.h"

namespace scram {

/// Facilities to report analysis results.
class Reporter {
 public:
  /// Reports the results of risk analysis on a model.
  /// The XML report is formed as a single document.
  ///
  /// @param[in] risk_an  Risk analysis with results.
  /// @param[out] out  The report destination stream.
  /// @param[in] indent  The flag to indent output for readability.
  ///
  /// @pre The output destination is used only by this reporter.
  ///      There is going to be no appending to the stream after the report.
  ///
  /// @throws IOError  The write operation has failed.
  void Report(const core::RiskAnalysis& risk_an, std::FILE* out,
              bool indent = true);

  /// A convenience function to generate the report into a file.
  /// This function overwrites the file.
  ///
  /// @param[in] risk_an  Risk analysis with results.
  /// @param[out] file  The output destination.
  /// @param[in] indent  The flag to indent output for readability.
  ///
  /// @throws IOError  The output file is not accessible,
  ///                  or the write operation has failed.
  void Report(const core::RiskAnalysis& risk_an, const std::string& file,
              bool indent = true);

 private:
  /// This function populates information
  /// about the software, settings, time, methods, model, etc.
  ///
  /// @param[in] risk_an  Risk analysis with all the information.
  /// @param[in,out] report  The root element of the document.
  void ReportInformation(const core::RiskAnalysis& risk_an,
                         xml::StreamElement* report);

  /// Reports software information and relevant run identifiers.
  ///
  /// @param[in,out] information  The XML element to append the results.
  void ReportSoftwareInformation(xml::StreamElement* information);

  /// Reports information about calculated quantities.
  /// The default call reports everything about the analysis
  /// as requested by settings.
  ///
  /// @tparam T  The kind of analysis information to be reported.
  ///
  /// @param[in] settings  The whole analysis settings.
  /// @param[in,out] information  The XML element to append the results.
  template <class T = core::RiskAnalysis>
  void ReportCalculatedQuantity(const core::Settings& settings,
                                xml::StreamElement* information);

  /// Reports summary of the model and its constructs.
  ///
  /// @param[in] model  The container of all the analysis constructs.
  /// @param[in,out] information  The XML element to append the results.
  void ReportModelFeatures(const mef::Model& model,
                           xml::StreamElement* information);

  /// Reports performance metrics of all conducted analyses.
  ///
  /// @param[in] risk_an  Risk analysis with all the performance information.
  /// @param[in,out] information  The XML element to append the results.
  void ReportPerformance(const core::RiskAnalysis& risk_an,
                         xml::StreamElement* information);

  /// Reports unused elements
  /// as warnings of the top information level.
  ///
  /// @tparam T  The range type of pointers to elements for forward traversal.
  ///            The element type in the container must provide usage().
  ///
  /// @param[in] container  The container with pointers to elements.
  /// @param[in] header  The header message to identify elements in the message.
  /// @param[in,out] information  The XML element to append the results.
  template <class T>
  void ReportUnusedElements(const T& container, const std::string& header,
                            xml::StreamElement* information);

  /// Reports the results of event tree analysis
  /// to a specified output destination.
  ///
  /// @param[in] eta_result  Event Tree Analysis with sequence results.
  /// @param[in,out] results  XML element to for all results.
  ///
  /// @pre The probability analysis has been performed.
  void ReportResults(const core::RiskAnalysis::EtaResult& eta_result,
                     xml::StreamElement* results);

  /// Reports the results of fault tree analysis
  /// to a specified output destination.
  ///
  /// @param[in] id  The analysis id.
  /// @param[in] fta  Fault Tree Analysis with results.
  /// @param[in] prob_analysis  Probability Analysis with results.
  ///                           Null pointer for no probability analysis.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(const core::RiskAnalysis::Result::Id& id,
                     const core::FaultTreeAnalysis& fta,
                     const core::ProbabilityAnalysis* prob_analysis,
                     xml::StreamElement* results);

  /// Reports results of probability analysis.
  ///
  /// @param[in] id  The analysis id.
  /// @param[in] prob_analysis  Probability Analysis with results.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(const core::RiskAnalysis::Result::Id& id,
                     const core::ProbabilityAnalysis& prob_analysis,
                     xml::StreamElement* results);

  /// Reports results of importance analysis.
  ///
  /// @param[in] id  The analysis id.
  /// @param[in] importance_analysis  Importance analysis with results.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(const core::RiskAnalysis::Result::Id& id,
                     const core::ImportanceAnalysis& importance_analysis,
                     xml::StreamElement* results);

  /// Reports the results of uncertainty analysis.
  ///
  /// @param[in] id  The analysis id.
  /// @param[in] uncert_analysis  Uncertainty analysis with results.
  /// @param[in,out] results  XML element to for all results.
  void ReportResults(const core::RiskAnalysis::Result::Id& id,
                     const core::UncertaintyAnalysis& uncert_analysis,
                     xml::StreamElement* results);

  /// Reports literal in products.
  ///
  /// @param[in] literal  A literal to be reported.
  /// @param[in,out] parent  A parent element node to have this literal.
  void ReportLiteral(const core::Literal& literal, xml::StreamElement* parent);

  /// Detects if a given basic event is a CCF event,
  /// and reports it with specific formatting.
  ///
  /// @tparam T  Function object type with xml::StreamElement* param.
  ///
  /// @param[in] basic_event  A basic event to be reported.
  /// @param[in,out] parent  A parent element node to have this basic event.
  /// @param[in] add_data  An additional function to append extra data.
  ///
  /// @note The additional function is given
  ///       due to the streaming nature of the XML reporting.
  ///       No information can be added after the stream is closed.
  ///       The callback function puts the extra information in-between.
  template <class T>
  void ReportBasicEvent(const mef::BasicEvent& basic_event,
                        xml::StreamElement* parent, const T& add_data);
};

}  // namespace scram

#endif  // SCRAM_SRC_REPORTER_H_
