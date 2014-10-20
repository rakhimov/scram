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
  friend class RiskAnalysis;

 public:
  Settings();

  /// Sets the limit order for minimal cut sets.
  /// @param[in] order A natural number for the limit order.
  /// @returns Rerefence to this object.
  /// @throws ValueError if the number is not more than 0.
  Settings& limit_order(int order);

  /// Limits the number of sums in probability calculations.
  /// @param[in] n A natural number for the number of sums.
  /// @returns Rerefence to this object.
  /// @throws ValueError if the number is not more than 0.
  Settings& num_sums(int n);

  /// Sets the cut-off probability for minimal cut sets to be considered
  /// for analysis.
  /// @param[in] prob The minimum probability for minimal cut sets.
  /// @returns Rerefence to this object.
  /// @throws ValueError if the probabilyt is not in the [0, 1] range.
  Settings& cut_off(double prob);

  /// Sets the approximation for probability analysis.
  /// @param[in] approx Approximation to be applied.
  /// @returns Rerefence to this object.
  /// @throws ValueError if the approximation is not recognized.
  Settings& approx(std::string approx);

  /// Sets the type of fault tree analysis.
  /// @param[in] analysis A type of analysis: default or mc.
  /// @returns Rerefence to this object.
  /// @throws ValueError if the analysis is not recognized.
  Settings& fta_type(std::string analysis);

  /// Sets the number of trials for Monte Carlo simulations.
  /// @param[in] trials A positive number.
  /// @returns Rerefence to this object.
  Settings& trials(int trials);

  /// Sets the system mission time.
  /// @param[in] time A positive number in hours by default.
  /// @returns Rerefence to this object.
  Settings& mission_time(double time);

 private:
  /// Limit on the order of minimal cut sets.
  int limit_order_;

  /// The number of sums in probability calculation series.
  int num_sums_;

  /// The cut-off probability for minimal cut sets to be in calculations.
  double cut_off_;

  /// The approximation to be applied for calculations.
  std::string approx_;

  /// The type of fault tree analysis.
  std::string fta_type_;

  /// The number of trials for Monte Carlo simulations.
  int trials_;

  /// System mission time.
  double mission_time_;
};

}  // namespace scram

#endif   // SCRAM_SRC_SETTINGS_H_
