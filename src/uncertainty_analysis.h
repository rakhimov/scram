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

/// @file uncertainty_analysis.h
/// Provides functionality for uncertainty analysis
/// with Monte Carlo method.

#ifndef SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
#define SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_

#include <utility>
#include <vector>

#include "analysis.h"
#include "probability_analysis.h"
#include "settings.h"

namespace scram {

namespace mef {  // Decouple from the implementation dependence.
class Expression;
}  // namespace mef

namespace core {

/// Uncertainty analysis and statistics
/// for top event or gate probabilities
/// with probability distributions of basic events.
class UncertaintyAnalysis : public Analysis {
 public:
  /// Uncertainty analysis
  /// on the fault tree processed
  /// by probability analysis.
  ///
  /// @param[in] prob_analysis  Completed probability analysis.
  explicit UncertaintyAnalysis(const ProbabilityAnalysis* prob_analysis);

  virtual ~UncertaintyAnalysis() = default;

  /// Performs quantitative analysis on the total probability.
  ///
  /// @note  Undefined behavior if analysis called two or more times.
  void Analyze() noexcept;

  /// @returns Mean of the final distribution.
  double mean() const { return mean_; }

  /// @returns Standard deviation of the final distribution.
  double sigma() const { return sigma_; }

  /// @returns Error factor for 95% confidence level.
  double error_factor() const { return error_factor_; }

  /// @returns 95% confidence interval of the mean.
  const std::pair<double, double>& confidence_interval() const {
    return confidence_interval_;
  }

  /// @returns The distribution histogram.
  const std::vector<std::pair<double, double> >& distribution() const {
    return distribution_;
  }

  /// @returns Quantiles of the distribution.
  const std::vector<double>& quantiles() const { return quantiles_; }

 protected:
  /// Gathers deviate expressions of variables.
  ///
  /// @param[in] graph  PDAG with the variables.
  ///
  /// @returns The gathered deviate expressions with variable indices.
  std::vector<std::pair<int, mef::Expression&>> GatherDeviateExpressions(
      const Pdag* graph) noexcept;

  /// Samples uncertain probabilities.
  ///
  /// @param[in] deviate_expressions  A collection of deviate expressions.
  /// @param[in,out] p_vars  Indices to probabilities mapping with values.
  void SampleExpressions(
      const std::vector<std::pair<int, mef::Expression&>>& deviate_expressions,
      Pdag::IndexMap<double>* p_vars) noexcept;

 private:
  /// Performs Monte Carlo Simulation
  /// by sampling the probability distributions
  /// and providing the final sampled values of the final probability.
  ///
  /// @returns Sampled values.
  virtual std::vector<double> Sample() noexcept = 0;

  /// Calculates statistical values from the final distribution.
  ///
  /// @param[in] samples  Gathered samples for statistical analysis.
  void CalculateStatistics(const std::vector<double>& samples) noexcept;

  double mean_;  ///< The mean of the final distribution.
  double sigma_;  ///< The standard deviation of the final distribution.
  double error_factor_;  ///< Error factor for 95% confidence level.
  /// The confidence interval of the distribution.
  std::pair<double, double> confidence_interval_;
  /// The histogram density of the distribution with lower bounds and values.
  std::vector<std::pair<double, double>> distribution_;
  /// The quantiles of the distribution.
  std::vector<double> quantiles_;
};

/// Uncertainty analysis facility.
///
/// @tparam Calculator  Quantitative analysis calculator.
template <class Calculator>
class UncertaintyAnalyzer : public UncertaintyAnalysis {
 public:
  /// Constructs uncertainty analyzer from probability analyzer.
  /// Probability analyzer facilities are used
  /// to calculate the total probability for sampling.
  ///
  /// @param[in] prob_analyzer  Instantiated probability analyzer.
  explicit UncertaintyAnalyzer(ProbabilityAnalyzer<Calculator>* prob_analyzer)
      : UncertaintyAnalysis(prob_analyzer),
        prob_analyzer_(prob_analyzer) {}

 private:
  /// @returns Samples of the total probability.
  std::vector<double> Sample() noexcept override;

  /// Calculator of the total probability.
  ProbabilityAnalyzer<Calculator>* prob_analyzer_;
};

template <class Calculator>
std::vector<double> UncertaintyAnalyzer<Calculator>::Sample() noexcept {
  std::vector<std::pair<int, mef::Expression&>> deviate_expressions =
      UncertaintyAnalysis::GatherDeviateExpressions(prob_analyzer_->graph());
  Pdag::IndexMap<double> p_vars = prob_analyzer_->p_vars();  // Private copy!
  std::vector<double> samples;
  samples.reserve(Analysis::settings().num_trials());

  for (int i = 0; i < Analysis::settings().num_trials(); ++i) {
    UncertaintyAnalysis::SampleExpressions(deviate_expressions, &p_vars);
    double result = prob_analyzer_->CalculateTotalProbability(p_vars);
    assert(result >= 0 && result <= 1);
    samples.push_back(result);
  }

  return samples;
}

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
