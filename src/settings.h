/// @file settings.h
/// Builder for settings.
#ifndef SCRAM_SRC_SETTINGS_H_
#define SCRAM_SRC_SETTINGS_H_

#include <string>

namespace scram {

class RiskAnalysis;

/// @class Settings
/// Builder for analysis settings.
class Settings {
  friend class Initializer;
  friend class RiskAnalysis;
  friend class Reporter;

 public:
  Settings();

  /// Sets the limit order for minimal cut sets.
  /// @param[in] order A natural number for the limit order.
  /// @returns Reference to this object.
  /// @throws ValueError if the number is not more than 0.
  Settings& limit_order(int order);

  /// Limits the number of sums in probability calculations.
  /// @param[in] n A natural number for the number of sums.
  /// @returns Reference to this object.
  /// @throws ValueError if the number is less than 1.
  Settings& num_sums(int n);

  /// Sets the cut-off probability for minimal cut sets to be considered
  /// for analysis.
  /// @param[in] prob The minimum probability for minimal cut sets.
  /// @returns Reference to this object.
  /// @throws ValueError if the probability is not in the [0, 1] range.
  Settings& cut_off(double prob);

  /// Sets the approximation for probability analysis.
  /// @param[in] approx Approximation to be applied.
  /// @returns Reference to this object.
  /// @throws ValueError if the approximation is not recognized.
  Settings& approx(std::string approx);

  /// Sets the number of trials for Monte Carlo simulations.
  /// @param[in] n A natural number for the number of trials.
  /// @returns Reference to this object.
  /// @throws ValueError if the number is less than 1.
  Settings& num_trials(int n);

  /// Sets the seed for the pseudo-random number generator.
  /// @param[in] s A positive number.
  /// @returns Reference to this object.
  /// @throws ValueError if the number is negative.
  Settings& seed(int s);

  /// Sets the system mission time.
  /// @param[in] time A positive number in hours by default.
  /// @returns Reference to this object.
  Settings& mission_time(double time);

  /// Sets the flag for probability analysis. If another analysis requires
  /// probability analysis, it won't be possible to turn off probability
  /// analysis before the parent analysis.
  /// @param[in] flag True or false for turning on or off the analysis.
  /// @returns Reference to this object.
  Settings& probability_analysis(bool flag) {
    if (!importance_analysis_ && !uncertainty_analysis_)
      probability_analysis_ = flag;
    return *this;
  }

  /// Sets the flag for importance analysis. Importance analysis is performed
  /// together with probability analysis. Appropriate flags are turned on.
  /// @param[in] flag True or false for turning on or off the analysis.
  /// @returns Reference to this object.
  Settings& importance_analysis(bool flag) {
    importance_analysis_ = flag;
    if (importance_analysis_) probability_analysis_ = true;
    return *this;
  }

  /// Sets the flag for uncertainty analysis. Uncertainty analysis implies
  /// probability analysis, so the probability analysis is turned on implicitly.
  /// @param[in] flag True or false for turning on or off the analysis.
  /// @returns Reference to this object.
  Settings& uncertainty_analysis(bool flag) {
    uncertainty_analysis_ = flag;
    if (uncertainty_analysis_) probability_analysis_ = true;
    return *this;
  }

  /// Sets the flag for CCF analysis.
  /// @param[in] flag True or false for turning on or off the analysis.
  /// @returns Reference to this object.
  Settings& ccf_analysis(bool flag) {
    ccf_analysis_ = flag;
    return *this;
  }

  /// This comparison is primarily for testing.
  /// @param[in] rhs Another Settings object to be compared.
  bool operator==(const Settings& rhs) const {
    return (probability_analysis_ == rhs.probability_analysis_) &&
        (importance_analysis_ == rhs.importance_analysis_) &&
        (uncertainty_analysis_ == rhs.uncertainty_analysis_) &&
        (ccf_analysis_ == rhs.ccf_analysis_) &&
        (limit_order_ == rhs.limit_order_) &&
        (num_sums_ == rhs.num_sums_) &&
        (cut_off_ == rhs.cut_off_) &&
        (approx_ == rhs.approx_) &&
        (num_trials_ == rhs.num_trials_) &&
        (seed_ == rhs.seed_) &&
        (mission_time_ == rhs.mission_time_);
  }

 private:
  bool probability_analysis_;  ///< A flag for probability analysis.
  bool importance_analysis_;  ///< A flag for importance analysis.
  bool uncertainty_analysis_;  ///< A flag for uncertainty analysis.
  bool ccf_analysis_;  ///< A flag for common-cause analysis.
  int limit_order_;  ///< Limit on the order of minimal cut sets.
  int num_sums_;  ///< The number of sums in probability calculation series.
  double cut_off_;  ///< The cut-off probability for cut sets.
  std::string approx_;  ///< The approximation to be applied for calculations.
  int num_trials_;  ///< The number of trials for Monte Carlo simulations.
  int seed_;  ///< The seed for the pseudo-random number generator.
  double mission_time_;  ///< System mission time.
};

}  // namespace scram

#endif   // SCRAM_SRC_SETTINGS_H_
