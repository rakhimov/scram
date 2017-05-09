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

/// @file risk_analysis.h
/// Contains the main system for performing analysis.

#ifndef SCRAM_SRC_RISK_ANALYSIS_H_
#define SCRAM_SRC_RISK_ANALYSIS_H_

#include <memory>
#include <utility>
#include <vector>

#include <boost/variant.hpp>

#include "analysis.h"
#include "event.h"
#include "event_tree_analysis.h"
#include "fault_tree_analysis.h"
#include "importance_analysis.h"
#include "model.h"
#include "probability_analysis.h"
#include "settings.h"
#include "uncertainty_analysis.h"

namespace scram {
namespace core {

/// Main system that performs analyses.
class RiskAnalysis : public Analysis {
 public:
  /// The analysis results binding to the unique analysis target.
  struct Result {
    /// The analysis target type as a unique identifier.
    using Id =
        boost::variant<const mef::Gate*, std::pair<const mef::InitiatingEvent&,
                                                   const mef::Sequence&>>;
    const Id id;  ///< The main analysis input or target.

    /// Optional analyses, i.e., may be nullptr.
    /// @{
    std::unique_ptr<const FaultTreeAnalysis> fault_tree_analysis;
    std::unique_ptr<const ProbabilityAnalysis> probability_analysis;
    std::unique_ptr<const ImportanceAnalysis> importance_analysis;
    std::unique_ptr<const UncertaintyAnalysis> uncertainty_analysis;
    /// @}
  };

  /// @param[in] model  An analysis model with fault trees, events, etc.
  /// @param[in] settings  Analysis settings for the given model.
  RiskAnalysis(std::shared_ptr<const mef::Model> model,
               const Settings& settings);

  /// @returns The model under analysis.
  const mef::Model& model() const { return *model_; }

  /// Analyzes the model
  /// and performs computations specified in the settings.
  ///
  /// @note This function must be called
  ///       only after full initialization of the model
  ///       with or without its probabilities.
  ///
  /// @pre The analysis is performed only once.
  void Analyze() noexcept;

  /// @returns The results of the analysis.
  const std::vector<Result>& results() const { return results_; }

  /// @returns The results of the event tree analysis.
  const std::vector<std::unique_ptr<EventTreeAnalysis>>& event_tree_results()
      const {
    return event_tree_results_;
  }

 private:
  /// Runs all possible analysis on a given target.
  /// Analysis types are deduced from the settings.
  ///
  /// @param[in] target  Analysis target.
  /// @param[in,out] result  The result container element.
  void RunAnalysis(const mef::Gate& target, Result* result) noexcept;

  /// Defines and runs Qualitative analysis on the target.
  /// Calls the Quantitative analysis if requested in settings.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  ///
  /// @param[in] target  Analysis target.
  /// @param[in,out] result  The result container element.
  template <class Algorithm>
  void RunAnalysis(const mef::Gate& target, Result* result) noexcept;

  /// Defines and runs Quantitative analysis on the target.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  /// @tparam Calculator  Quantitative analysis algorithm.
  ///
  /// @param[in] fta  The result of Qualitative analysis.
  /// @param[in,out] result  The result container element.
  ///
  /// @pre FaultTreeAnalyzer is ready to tolerate
  ///      giving its internals to Quantitative analyzers.
  template <class Algorithm, class Calculator>
  void RunAnalysis(FaultTreeAnalyzer<Algorithm>* fta, Result* result) noexcept;

  std::shared_ptr<const mef::Model> model_;  ///< The model with constructs.
  std::vector<Result> results_;  ///< The analysis result storage.
  /// Event tree analysis of sequences.
  /// @todo Incorporate into the main results container.
  std::vector<std::unique_ptr<EventTreeAnalysis>> event_tree_results_;
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_RISK_ANALYSIS_H_
