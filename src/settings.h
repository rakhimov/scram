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
  /// Sets the limit order for minimal cut sets.
  ///
  /// @param[in] order A natural number for the limit order.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The number is not more than 0.
  Settings& limit_order(int order);

  /// Limits the number of sums in probability calculations.
  ///
  /// @param[in] n A natural number for the number of sums.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The number is less than 1.
  Settings& num_sums(int n);

  /// Sets the cut-off probability for minimal cut sets to be considered
  /// for analysis.
  ///
  /// @param[in] prob The minimum probability for minimal cut sets.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The probability is not in the [0, 1] range.
  Settings& cut_off(double prob);

  /// Sets the approximation for probability analysis.
  ///
  /// @param[in] approx Approximation to be applied.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The approximation is not recognized.
  Settings& approx(const std::string& approx);

  /// Sets the number of trials for Monte Carlo simulations.
  ///
  /// @param[in] n A natural number for the number of trials.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The number is less than 1.
  Settings& num_trials(int n);

  /// Sets the number of quantiles for distributions.
  ///
  /// @param[in] n A natural number for the number of quantiles.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The number is less than 1.
  Settings& num_quantiles(int n);

  /// Sets the number of bins for histograms.
  ///
  /// @param[in] n A natural number for the number of bins.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The number is less than 1.
  Settings& num_bins(int n);

  /// Sets the seed for the pseudo-random number generator.
  ///
  /// @param[in] s A positive number.
  ///
  /// @returns Reference to this object.
  ///
  /// @throws ValueError The number is negative.
  Settings& seed(int s);

  /// Sets the system mission time.
  ///
  /// @param[in] time A positive number in hours by default.
  ///
  /// @returns Reference to this object.
  Settings& mission_time(double time);

  /// Sets the flag for probability analysis.
  /// If another analysis requires probability analysis,
  /// it won't be possible to turn off probability analysis
  /// before the parent analysis.
  ///
  /// @param[in] flag True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& probability_analysis(bool flag) {
    if (!importance_analysis_ && !uncertainty_analysis_)
      probability_analysis_ = flag;
    return *this;
  }

  /// Sets the flag for importance analysis.
  /// Importance analysis is performed
  /// together with probability analysis.
  /// Appropriate flags are turned on.
  ///
  /// @param[in] flag True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& importance_analysis(bool flag) {
    importance_analysis_ = flag;
    if (importance_analysis_) probability_analysis_ = true;
    return *this;
  }

  /// Sets the flag for uncertainty analysis.
  /// Uncertainty analysis implies probability analysis,
  /// so the probability analysis is turned on implicitly.
  ///
  /// @param[in] flag True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& uncertainty_analysis(bool flag) {
    uncertainty_analysis_ = flag;
    if (uncertainty_analysis_) probability_analysis_ = true;
    return *this;
  }

  /// Sets the flag for CCF analysis.
  ///
  /// @param[in] flag True or false for turning on or off the analysis.
  ///
  /// @returns Reference to this object.
  Settings& ccf_analysis(bool flag) {
    ccf_analysis_ = flag;
    return *this;
  }

  inline bool probability_analysis() const { return probability_analysis_; }
  inline bool importance_analysis() const { return importance_analysis_; }
  inline bool uncertainty_analysis() const { return uncertainty_analysis_; }
  inline bool ccf_analysis() const { return ccf_analysis_; }
  inline int limit_order() const { return limit_order_; }
  inline double mission_time() const { return mission_time_; }
  inline int num_sums() const { return num_sums_; }
  inline double cut_off() const { return cut_off_; }
  inline const std::string& approx() const { return approx_; }
  inline int seed() const { return seed_; }
  inline int num_trials() const { return num_trials_; }
  inline int num_quantiles() const { return num_quantiles_; }
  inline int num_bins() const { return num_bins_; }

 private:
  bool probability_analysis_ = false;  ///< A flag for probability analysis.
  bool importance_analysis_ = false;  ///< A flag for importance analysis.
  bool uncertainty_analysis_ = false;  ///< A flag for uncertainty analysis.
  bool ccf_analysis_ = false;  ///< A flag for common-cause analysis.
  int limit_order_ = 20;  ///< Limit on the order of minimal cut sets.
  double mission_time_ = 8760;  ///< System mission time.
  int num_sums_ = 7;  ///< The number of sums in probability calculation series.
  double cut_off_ = 1e-8;  ///< The cut-off probability for cut sets.
  std::string approx_ = "no";  ///< The approximations for calculations.
  int seed_ = 0;  ///< The seed for the pseudo-random number generator.
  int num_trials_ = 1e3;  ///< The number of trials for Monte Carlo simulations.
  int num_quantiles_ = 20;  ///< The number of quantiles for distributions.
  int num_bins_ = 20;  ///< The number of bins for histograms.
};

}  // namespace scram

#endif   // SCRAM_SRC_SETTINGS_H_
