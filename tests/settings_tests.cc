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

#include "settings.h"

#include <catch2/catch.hpp>

#include "error.h"

namespace scram::core::test {

TEST_CASE("SettingsTest IncorrectSetup", "[settings]") {
  Settings s;
  // Incorrect algorithm.
  CHECK_THROWS_AS(s.algorithm("the-best"), SettingsError);
  // Incorrect approximation argument.
  CHECK_THROWS_AS(s.approximation("approx"), SettingsError);
  // Incorrect limit order for products.
  CHECK_THROWS_AS(s.limit_order(-1), SettingsError);
  // Incorrect cut-off probability.
  CHECK_THROWS_AS(s.cut_off(-1), SettingsError);
  CHECK_THROWS_AS(s.cut_off(10), SettingsError);
  // Incorrect number of trials.
  CHECK_THROWS_AS(s.num_trials(-10), SettingsError);
  CHECK_THROWS_AS(s.num_trials(0), SettingsError);
  // Incorrect number of quantiles.
  CHECK_THROWS_AS(s.num_quantiles(-10), SettingsError);
  CHECK_THROWS_AS(s.num_quantiles(0), SettingsError);
  // Incorrect number of bins.
  CHECK_THROWS_AS(s.num_bins(-10), SettingsError);
  CHECK_THROWS_AS(s.num_bins(0), SettingsError);
  // Incorrect seed.
  CHECK_THROWS_AS(s.seed(-1), SettingsError);
  // Incorrect mission time.
  CHECK_THROWS_AS(s.mission_time(-10), SettingsError);
  // Incorrect time step.
  CHECK_THROWS_AS(s.time_step(-1), SettingsError);
  // The time step is not set for the SIL calculations.
  CHECK_THROWS_AS(s.safety_integrity_levels(true), SettingsError);
  // Disable time step while the SIL is requested.
  CHECK_NOTHROW(s.time_step(1));
  CHECK_NOTHROW(s.safety_integrity_levels(true));
  CHECK_THROWS_AS(s.time_step(0), SettingsError);
}

TEST_CASE("SettingsTest CorrectSetup", "[settings]") {
  Settings s;
  // Correct algorithm.
  CHECK_NOTHROW(s.algorithm("mocus"));
  CHECK_NOTHROW(s.algorithm("bdd"));
  CHECK_NOTHROW(s.algorithm("zbdd"));

  // Correct approximation argument.
  CHECK_NOTHROW(s.approximation("rare-event"));
  CHECK_NOTHROW(s.approximation("mcub"));

  // Correct limit order for products.
  CHECK_NOTHROW(s.limit_order(1));
  CHECK_NOTHROW(s.limit_order(32));
  CHECK_NOTHROW(s.limit_order(1e9));

  // Correct cut-off probability.
  CHECK_NOTHROW(s.cut_off(1));
  CHECK_NOTHROW(s.cut_off(0));
  CHECK_NOTHROW(s.cut_off(0.5));

  // Correct number of trials.
  CHECK_NOTHROW(s.num_trials(1));
  CHECK_NOTHROW(s.num_trials(1e6));

  // Correct number of quantiles.
  CHECK_NOTHROW(s.num_quantiles(1));
  CHECK_NOTHROW(s.num_quantiles(10));

  // Correct number of bins.
  CHECK_NOTHROW(s.num_bins(1));
  CHECK_NOTHROW(s.num_bins(10));

  // Correct seed.
  CHECK_NOTHROW(s.seed(1));

  // Correct mission time.
  CHECK_NOTHROW(s.mission_time(0));
  CHECK_NOTHROW(s.mission_time(10));
  CHECK_NOTHROW(s.mission_time(1e6));

  // Correct time step.
  CHECK_NOTHROW(s.time_step(0));
  CHECK_NOTHROW(s.time_step(10));
  CHECK_NOTHROW(s.time_step(1e6));

  // Correct request for the SIL.
  CHECK_NOTHROW(s.safety_integrity_levels(true));
  CHECK_NOTHROW(s.safety_integrity_levels(false));
}

TEST_CASE("SettingsTest SetupForPrimeImplicants", "[settings]") {
  Settings s;
  // Incorrect request for prime implicants.
  CHECK_NOTHROW(s.algorithm("mocus"));
  CHECK_THROWS_AS(s.prime_implicants(true), SettingsError);
  // Correct request for prime implicants.
  REQUIRE_NOTHROW(s.algorithm("bdd"));
  REQUIRE_NOTHROW(s.prime_implicants(true));
  // Prime implicants with quantitative approximations.
  CHECK_NOTHROW(s.approximation("none"));
  CHECK_THROWS_AS(s.approximation("rare-event"), SettingsError);
  CHECK_THROWS_AS(s.approximation("mcub"), SettingsError);
}

}  // namespace scram::core::test
