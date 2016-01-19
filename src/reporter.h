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
class Parameter;
class PrimaryEvent;
class BasicEvent;
class FaultTreeAnalysis;
class ProbabilityAnalysis;
class ImportanceAnalysis;
class UncertaintyAnalysis;

/// @class Reporter
/// This class reports the results of the analyses.
class Reporter {
 public:
  using PrimaryEventPtr = std::shared_ptr<const PrimaryEvent>;  ///< @todo Ugly!

  /// Sets up XML report document according to a specific standards.
  /// This function populates information
  /// about the software, settings, time, methods, and model.
  /// In addition, the function forms the structure
  /// of the overall report for use by other reporting functions.
  /// This function must be called before other reporting functions.
  ///
  /// @param[in] model  The main model container.
  /// @param[in] settings  Configured settings for analysis.
  /// @param[in,out] doc  An empty document.
  ///
  /// @throws LogicError  The document is not empty.
  void SetupReport(const std::shared_ptr<const Model>& model,
                   const Settings& settings,
                   xmlpp::Document* doc);

  /// Reports orphan primary events
  /// as warnings of the top information level.
  ///
  /// @param[in] orphan_primary_events  Container of orphan events.
  /// @param[in,out] doc  Pre-formatted XML document.
  void ReportOrphanPrimaryEvents(
      const std::vector<PrimaryEventPtr>& orphan_primary_events,
      xmlpp::Document* doc);

  /// Reports unused parameters
  /// as warnings of the top information level.
  ///
  /// @param[in] unused_parameters  Container of unused parameters.
  /// @param[in,out] doc  Pre-formatted XML document.
  void ReportUnusedParameters(
      const std::vector<std::shared_ptr<const Parameter>>& unused_parameters,
      xmlpp::Document* doc);

  /// Reports the results of analysis
  /// to a specified output destination.
  ///
  /// @param[in] ft_name  The original name of a fault tree.
  /// @param[in] fta  Fault Tree Analysis with results.
  /// @param[in] prob_analysis  ProbabilityAnalysis with results.
  ///                           Null pointer for no probability analysis.
  /// @param[in,out] doc  Pre-formatted XML document.
  ///
  /// @note This function must be called only after analysis is done.
  void ReportFta(std::string ft_name, const FaultTreeAnalysis& fta,
                 const ProbabilityAnalysis* prob_analysis,
                 xmlpp::Document* doc);

  /// Reports results of importance analysis in probability analysis.
  ///
  /// @param[in] ft_name  The original name of a fault tree.
  /// @param[in] importance_analysis  ImportanceAnalysis with results.
  /// @param[in,out] doc  Pre-formatted XML document.
  ///
  /// @note This function must be called only after analysis is done.
  void ReportImportance(std::string ft_name,
                        const ImportanceAnalysis& importance_analysis,
                        xmlpp::Document* doc);

  /// Reports the results of uncertainty analysis.
  ///
  /// @param[in] ft_name  The original name of a fault tree.
  /// @param[in] uncert_analysis  UncertaintyAnalysis with results.
  /// @param[in,out] doc  Pre-formatted XML document.
  ///
  /// @note This function must be called only after analysis is done.
  void ReportUncertainty(std::string ft_name,
                         const UncertaintyAnalysis& uncert_analysis,
                         xmlpp::Document* doc);

 private:
  using BasicEventPtr = std::shared_ptr<const BasicEvent>;  ///< For simplicity.

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
