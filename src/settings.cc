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

/// @file settings.cc
/// Implementation of Settings Builder.

#include "settings.h"

#include <boost/range/algorithm.hpp>

#include "error.h"

namespace scram {
namespace core {

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

Settings& Settings::algorithm(const std::string& value) {
  auto it = boost::find(kAlgorithmToString, value);
  if (it == std::end(kAlgorithmToString))
    throw InvalidArgument("The qualitative analysis algorithm '" + value +
                          "' is not recognized.");
  return algorithm(
      static_cast<Algorithm>(std::distance(kAlgorithmToString, it)));
}

Settings& Settings::approximation(Approximation value) {
  if (value != Approximation::kNone && prime_implicants_)
    throw InvalidArgument(
        "Prime implicants require no quantitative approximation.");
  approximation_ = value;
  return *this;
}

Settings& Settings::approximation(const std::string& value) {
  auto it = boost::find(kApproximationToString, value);
  if (it == std::end(kApproximationToString))
    throw InvalidArgument("The probability approximation '" + value +
                          "'is not recognized.");
  return approximation(
      static_cast<Approximation>(std::distance(kApproximationToString, it)));
}

Settings& Settings::prime_implicants(bool flag) {
  if (flag && algorithm_ != Algorithm::kBdd)
    throw InvalidArgument("Prime implicants can only be calculated with BDD");

  prime_implicants_ = flag;
  if (prime_implicants_)
    approximation(Approximation::kNone);
  return *this;
}

Settings& Settings::limit_order(int order) {
  if (order < 0)
    throw InvalidArgument("The limit on the order of products "
                          "cannot be less than 0.");
  limit_order_ = order;
  return *this;
}

Settings& Settings::cut_off(double prob) {
  if (prob < 0 || prob > 1)
    throw InvalidArgument("The cut-off probability cannot be negative or"
                          " more than 1.");
  cut_off_ = prob;
  return *this;
}

Settings& Settings::num_trials(int n) {
  if (n < 1)
    throw InvalidArgument("The number of trials cannot be less than 1.");

  num_trials_ = n;
  return *this;
}

Settings& Settings::num_quantiles(int n) {
  if (n < 1)
    throw InvalidArgument("The number of quantiles cannot be less than 1.");

  num_quantiles_ = n;
  return *this;
}

Settings& Settings::num_bins(int n) {
  if (n < 1)
    throw InvalidArgument("The number of bins cannot be less than 1.");

  num_bins_ = n;
  return *this;
}

Settings& Settings::seed(int s) {
  if (s < 0)
    throw InvalidArgument("The seed for PRNG cannot be negative.");

  seed_ = s;
  return *this;
}

Settings& Settings::mission_time(double time) {
  if (time < 0)
    throw InvalidArgument("The mission time cannot be negative.");

  mission_time_ = time;
  return *this;
}

Settings& Settings::time_step(double time) {
  if (time < 0)
    throw InvalidArgument("The time step cannot be negative.");
  if (!time && safety_integrity_levels_)
    throw InvalidArgument("The time step cannot be disabled for the SIL");

  time_step_ = time;
  return *this;
}

Settings& Settings::safety_integrity_levels(bool flag) {
  if (flag && !time_step_)
    throw InvalidArgument("The time step is not set for the SIL calculations.");

  safety_integrity_levels_ = flag;
  if (safety_integrity_levels_)
    probability_analysis_ = true;
  return *this;
}

}  // namespace core
}  // namespace scram
