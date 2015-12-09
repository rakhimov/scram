/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

/// @file settings.h
/// Builder for settings.

#ifndef SCRAM_SRC_SETTINGS_H_
#define SCRAM_SRC_SETTINGS_H_

#include <string>

namespace scram {

/// @class Settings
/// Builder for analysis settings.
class Settings {
 public:
  /// @returns The Qualitative analysis algorithm.
  const std::string& algorithm() const { return algorithm_; }

  /// Sets the algorithm for Qualitative analysis.
  ///
  /// @param[in] algorithm  The name of the algorithm in lower case.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The algorithm is not recognized.
  Settings& algorithm(const std::string& algorithm);

  /// @returns The limit on the size of minimal cut sets.
  int limit_order() const { return limit_order_; }

  /// Sets the limit order for minimal cut sets.
  ///
  /// @param[in] order  A natural number for the limit order.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The number is not more than 0.
  Settings& limit_order(int order);

  /// @returns The minimum required probability for cut sets.
  double cut_off() const { return cut_off_; }

  /// Sets the cut-off probability for minimal cut sets to be considered
  /// for analysis.
  ///
  /// @param[in] prob  The minimum probability for minimal cut sets.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The probability is not in the [0, 1] range.
  Settings& cut_off(double prob);

  /// @returns "no" if no quantitative approximation is requested.
  /// @returns "rare-event" for the rare-event approximation.
  /// @returns "mcub" for the min-cut-upper bound approximation.
  const std::string& approximation() const { return approximation_; }

  /// Sets the approximation for quantitative analysis.
  ///
  /// @param[in] approx  Approximation to be applied.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The approximation is not recognized.
  Settings& approximation(const std::string& approx);

  /// @returns The number of trials for Monte-Carlo simulations.
  int num_trials() const { return num_trials_; }

  /// Sets the number of trials for Monte Carlo simulations.
  ///
  /// @param[in] n  A natural number for the number of trials.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The number is less than 1.
  Settings& num_trials(int n);

  /// @returns The number of quantiles for distributions.
  int num_quantiles() const { return num_quantiles_; }

  /// Sets the number of quantiles for distributions.
  ///
  /// @param[in] n  A natural number for the number of quantiles.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The number is less than 1.
  Settings& num_quantiles(int n);

  /// @returns The number of bins for histograms.
  int num_bins() const { return num_bins_; }

  /// Sets the number of bins for histograms.
  ///
  /// @param[in] n  A natural number for the number of bins.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The number is less than 1.
  Settings& num_bins(int n);

  /// @returns The seed of the pseudo-random number generator.
  int seed() const { return seed_; }

  /// Sets the seed for the pseudo-random number generator.
  ///
  /// @param[in] s  A positive number.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws InvalidArgument  The number is negative.
  Settings& seed(int s);

  /// @returns The length time of the system under risk.
  double mission_time() const { return mission_time_; }

  /// Sets the system mission time.
  ///
  /// @param[in] time  A positive number in hours by default.
  ///
  /// @returns Reference to this object.
  Settings& mission_time(double time);

  /// @returns true if probability analysis is requested.
  bool probability_analysis() const { return probability_analysis_; }

  /// Sets the flag for probability analysis.
  /// If another analysis requires probability analysis,
  /// it won't be possible to turn off probability analysis
  /// before the parent analysis.
  ///
  /// @param[in] flag  True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& probability_analysis(bool flag) {
    if (!importance_analysis_ && !uncertainty_analysis_)
      probability_analysis_ = flag;
    return *this;
  }

  /// @returns true if importance analysis is requested.
  bool importance_analysis() const { return importance_analysis_; }

  /// Sets the flag for importance analysis.
  /// Importance analysis is performed
  /// together with probability analysis.
  /// Appropriate flags are turned on.
  ///
  /// @param[in] flag  True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& importance_analysis(bool flag) {
    importance_analysis_ = flag;
    if (importance_analysis_) probability_analysis_ = true;
    return *this;
  }

  /// @returns true if uncertainty analysis is requested.
  bool uncertainty_analysis() const { return uncertainty_analysis_; }

  /// Sets the flag for uncertainty analysis.
  /// Uncertainty analysis implies probability analysis,
  /// so the probability analysis is turned on implicitly.
  ///
  /// @param[in] flag  True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& uncertainty_analysis(bool flag) {
    uncertainty_analysis_ = flag;
    if (uncertainty_analysis_) probability_analysis_ = true;
    return *this;
  }

  /// @returns true if CCF groups must be incorporated into analysis.
  bool ccf_analysis() const { return ccf_analysis_; }

  /// Sets the flag for CCF analysis.
  ///
  /// @param[in] flag  True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& ccf_analysis(bool flag) {
    ccf_analysis_ = flag;
    return *this;
  }

 private:
  bool probability_analysis_ = false;  ///< A flag for probability analysis.
  bool importance_analysis_ = false;  ///< A flag for importance analysis.
  bool uncertainty_analysis_ = false;  ///< A flag for uncertainty analysis.
  bool ccf_analysis_ = false;  ///< A flag for common-cause analysis.
  std::string algorithm_ = "bdd";  ///< Qualitative analysis algorithm.
  int limit_order_ = 20;  ///< Limit on the order of minimal cut sets.
  double mission_time_ = 8760;  ///< System mission time.
  double cut_off_ = 1e-8;  ///< The cut-off probability for cut sets.
  std::string approximation_ = "no";  ///< The approximations for calculations.
  int seed_ = 0;  ///< The seed for the pseudo-random number generator.
  int num_trials_ = 1e3;  ///< The number of trials for Monte Carlo simulations.
  int num_quantiles_ = 20;  ///< The number of quantiles for distributions.
  int num_bins_ = 20;  ///< The number of bins for histograms.
};

}  // namespace scram

#endif   // SCRAM_SRC_SETTINGS_H_
