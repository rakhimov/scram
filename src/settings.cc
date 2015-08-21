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

/// @file settings.cc
/// Implementation of Settings Builder.

#include "settings.h"

#include "error.h"

namespace scram {

Settings& Settings::limit_order(int order) {
  if (order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than 1.";
    throw InvalidArgument(msg);
  }
  limit_order_ = order;
  return *this;
}

Settings& Settings::num_sums(int n) {
  if (n < 1) {
    std::string msg = "The number of sums in the probability calculation "
                      "cannot be less than 1";
    throw InvalidArgument(msg);
  }
  num_sums_ = n;
  return *this;
}

Settings& Settings::cut_off(double prob) {
  if (prob < 0 || prob > 1) {
    std::string msg = "The cut-off probability cannot be negative or"
                      " more than 1.";
    throw InvalidArgument(msg);
  }
  cut_off_ = prob;
  return *this;
}

Settings& Settings::approx(const std::string& approx) {
  if (approx != "no" && approx != "rare-event" && approx != "mcub") {
    std::string msg = "The probability approximation is not recognized.";
    throw InvalidArgument(msg);
  }
  approx_ = approx;
  return *this;
}

Settings& Settings::num_trials(int n) {
  if (n < 1) {
    std::string msg = "The number of trials cannot be less than 1.";
    throw InvalidArgument(msg);
  }
  num_trials_ = n;
  return *this;
}

Settings& Settings::num_quantiles(int n) {
  if (n < 1) {
    std::string msg = "The number of quantiles cannot be less than 1.";
    throw InvalidArgument(msg);
  }
  num_quantiles_ = n;
  return *this;
}

Settings& Settings::num_bins(int n) {
  if (n < 1) {
    std::string msg = "The number of bins cannot be less than 1.";
    throw InvalidArgument(msg);
  }
  num_bins_ = n;
  return *this;
}

Settings& Settings::seed(int s) {
  if (s < 0) {
    std::string msg = "The seed for PRNG cannot be negative.";
    throw InvalidArgument(msg);
  }
  seed_ = s;
  return *this;
}

Settings& Settings::mission_time(double time) {
  if (time < 0) {
    std::string msg = "The mission time cannot be negative.";
    throw InvalidArgument(msg);
  }
  mission_time_ = time;
  return *this;
}

}  // namespace scram
