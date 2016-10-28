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

/// @file risk_analysis.h
/// Contains the main system for performing analysis.

#ifndef SCRAM_SRC_RISK_ANALYSIS_H_
#define SCRAM_SRC_RISK_ANALYSIS_H_

#include <map>
#include <memory>
#include <string>

#include "analysis.h"
#include "event.h"
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
  /// A storage container type for analysis kinds
  /// with their ids as keys.
  template <class T>
  using AnalysisTable = std::map<std::string, std::unique_ptr<T>>;

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
  void Analyze() noexcept;

  /// @returns Containers of performed analyses
  ///          identified by the top gate identifiers.
  /// @{
  const AnalysisTable<FaultTreeAnalysis>& fault_tree_analyses() const {
    return fault_tree_analyses_;
  }
  const AnalysisTable<ProbabilityAnalysis>& probability_analyses() const {
    return probability_analyses_;
  }
  const AnalysisTable<ImportanceAnalysis>& importance_analyses() const {
    return importance_analyses_;
  }
  const AnalysisTable<UncertaintyAnalysis>& uncertainty_analyses() const {
    return uncertainty_analyses_;
  }
  /// @}

 private:
  /// Runs all possible analysis on a given target.
  /// Analysis types are deduced from the settings.
  ///
  /// @param[in] name  Identificator for analyses.
  /// @param[in] target  Analysis target.
  void RunAnalysis(const std::string& name, const mef::Gate& target) noexcept;

  /// Defines and runs Qualitative analysis on the target.
  /// Calls the Quantitative analysis if requested in settings.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  ///
  /// @param[in] name  Identificator for analyses.
  /// @param[in] target  Analysis target.
  template <class Algorithm>
  void RunAnalysis(const std::string& name, const mef::Gate& target) noexcept;

  /// Defines and runs Quantitative analysis on the target.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  /// @tparam Calculator  Quantitative analysis algorithm.
  ///
  /// @param[in] name  Identificator for analyses.
  /// @param[in] fta  The result of Qualitative analysis.
  ///
  /// @pre FaultTreeAnalyzer is ready to tolerate
  ///      giving its internals to Quantitative analyzers.
  template <class Algorithm, class Calculator>
  void RunAnalysis(const std::string& name,
                   FaultTreeAnalyzer<Algorithm>* fta) noexcept;

  /// The analysis model with constructs.
  std::shared_ptr<const mef::Model> model_;

  /// Analyses performed by this risk analysis run.
  /// @{
  AnalysisTable<FaultTreeAnalysis> fault_tree_analyses_;
  AnalysisTable<ProbabilityAnalysis> probability_analyses_;
  AnalysisTable<ImportanceAnalysis> importance_analyses_;
  AnalysisTable<UncertaintyAnalysis> uncertainty_analyses_;
  /// @}
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_RISK_ANALYSIS_H_
