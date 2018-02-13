/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of Settings Builder.

#include "settings.h"

#include <string>

#include <boost/range/algorithm.hpp>

#include "error.h"

namespace scram::core {

Settings& Settings::algorithm(Algorithm value) noexcept {
  algorithm_ = value;
  switch (algorithm_) {
    case Algorithm::kBdd:
      approximation(Approximation::kNone);
      break;
    default:
      if (approximation_ == Approximation::kNone)
        approximation(Approximation::kRareEvent);
      if (prime_implicants_)
        prime_implicants(false);
  }
  return *this;
}

Settings& Settings::algorithm(std::string_view value) {
  auto it = boost::find(kAlgorithmToString, value);
  if (it == std::end(kAlgorithmToString))
    SCRAM_THROW(
        SettingsError("The qualitative analysis algorithm is not recognized."))
        << errinfo_value(std::string(value));

  return algorithm(
      static_cast<Algorithm>(std::distance(kAlgorithmToString, it)));
}

Settings& Settings::approximation(Approximation value) {
  if (value != Approximation::kNone && prime_implicants_)
    SCRAM_THROW(SettingsError(
        "Prime implicants require no quantitative approximation."));
  approximation_ = value;
  return *this;
}

Settings& Settings::approximation(std::string_view value) {
  auto it = boost::find(kApproximationToString, value);
  if (it == std::end(kApproximationToString))
    SCRAM_THROW(
        SettingsError("The probability approximation is not recognized."))
        << errinfo_value(std::string(value));

  return approximation(
      static_cast<Approximation>(std::distance(kApproximationToString, it)));
}

Settings& Settings::prime_implicants(bool flag) {
  if (flag && algorithm_ != Algorithm::kBdd)
    SCRAM_THROW(
        SettingsError("Prime implicants can only be calculated with BDD"));

  prime_implicants_ = flag;
  if (prime_implicants_)
    approximation(Approximation::kNone);
  return *this;
}

Settings& Settings::limit_order(int order) {
  if (order < 0)
    SCRAM_THROW(SettingsError(
        "The limit on the order of products cannot be less than 0."))
        << errinfo_value(std::to_string(order));

  limit_order_ = order;
  return *this;
}

Settings& Settings::cut_off(double prob) {
  if (prob < 0 || prob > 1)
    SCRAM_THROW(SettingsError(
        "The cut-off probability cannot be negative or more than 1."))
        << errinfo_value(std::to_string(prob));

  cut_off_ = prob;
  return *this;
}

Settings& Settings::num_trials(int n) {
  if (n < 1)
    SCRAM_THROW(SettingsError("The number of trials cannot be less than 1."))
        << errinfo_value(std::to_string(n));

  num_trials_ = n;
  return *this;
}

Settings& Settings::num_quantiles(int n) {
  if (n < 1)
    SCRAM_THROW(SettingsError("The number of quantiles cannot be less than 1."))
        << errinfo_value(std::to_string(n));

  num_quantiles_ = n;
  return *this;
}

Settings& Settings::num_bins(int n) {
  if (n < 1)
    SCRAM_THROW(SettingsError("The number of bins cannot be less than 1."))
        << errinfo_value(std::to_string(n));

  num_bins_ = n;
  return *this;
}

Settings& Settings::seed(int s) {
  if (s < 0)
    SCRAM_THROW(SettingsError("The seed for PRNG cannot be negative."))
        << errinfo_value(std::to_string(s));

  seed_ = s;
  return *this;
}

Settings& Settings::mission_time(double time) {
  if (time < 0)
    SCRAM_THROW(SettingsError("The mission time cannot be negative."))
        << errinfo_value(std::to_string(time));

  mission_time_ = time;
  return *this;
}

Settings& Settings::time_step(double time) {
  if (time < 0)
    SCRAM_THROW(SettingsError("The time step cannot be negative."))
        << errinfo_value(std::to_string(time));
  if (!time && safety_integrity_levels_)
    SCRAM_THROW(SettingsError("The time step cannot be disabled for the SIL"))
        << errinfo_value(std::to_string(time));

  time_step_ = time;
  return *this;
}

Settings& Settings::safety_integrity_levels(bool flag) {
  if (flag && !time_step_)
    SCRAM_THROW(
        SettingsError("The time step is not set for the SIL calculations."));

  safety_integrity_levels_ = flag;
  if (safety_integrity_levels_)
    probability_analysis_ = true;
  return *this;
}

}  // namespace scram::core
