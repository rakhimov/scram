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

#include <gtest/gtest.h>

#include "error.h"

namespace scram::core::test {

TEST(SettingsTest, IncorrectSetup) {
  Settings s;
  // Incorrect algorithm.
  EXPECT_THROW(s.algorithm("the-best"), SettingsError);
  // Incorrect approximation argument.
  EXPECT_THROW(s.approximation("approx"), SettingsError);
  // Incorrect limit order for products.
  EXPECT_THROW(s.limit_order(-1), SettingsError);
  // Incorrect cut-off probability.
  EXPECT_THROW(s.cut_off(-1), SettingsError);
  EXPECT_THROW(s.cut_off(10), SettingsError);
  // Incorrect number of trials.
  EXPECT_THROW(s.num_trials(-10), SettingsError);
  EXPECT_THROW(s.num_trials(0), SettingsError);
  // Incorrect number of quantiles.
  EXPECT_THROW(s.num_quantiles(-10), SettingsError);
  EXPECT_THROW(s.num_quantiles(0), SettingsError);
  // Incorrect number of bins.
  EXPECT_THROW(s.num_bins(-10), SettingsError);
  EXPECT_THROW(s.num_bins(0), SettingsError);
  // Incorrect seed.
  EXPECT_THROW(s.seed(-1), SettingsError);
  // Incorrect mission time.
  EXPECT_THROW(s.mission_time(-10), SettingsError);
  // Incorrect time step.
  EXPECT_THROW(s.time_step(-1), SettingsError);
  // The time step is not set for the SIL calculations.
  EXPECT_THROW(s.safety_integrity_levels(true), SettingsError);
  // Disable time step while the SIL is requested.
  EXPECT_NO_THROW(s.time_step(1));
  EXPECT_NO_THROW(s.safety_integrity_levels(true));
  EXPECT_THROW(s.time_step(0), SettingsError);
}

TEST(SettingsTest, CorrectSetup) {
  Settings s;
  // Correct algorithm.
  EXPECT_NO_THROW(s.algorithm("mocus"));
  EXPECT_NO_THROW(s.algorithm("bdd"));
  EXPECT_NO_THROW(s.algorithm("zbdd"));

  // Correct approximation argument.
  EXPECT_NO_THROW(s.approximation("rare-event"));
  EXPECT_NO_THROW(s.approximation("mcub"));

  // Correct limit order for products.
  EXPECT_NO_THROW(s.limit_order(1));
  EXPECT_NO_THROW(s.limit_order(32));
  EXPECT_NO_THROW(s.limit_order(1e9));

  // Correct cut-off probability.
  EXPECT_NO_THROW(s.cut_off(1));
  EXPECT_NO_THROW(s.cut_off(0));
  EXPECT_NO_THROW(s.cut_off(0.5));

  // Correct number of trials.
  EXPECT_NO_THROW(s.num_trials(1));
  EXPECT_NO_THROW(s.num_trials(1e6));

  // Correct number of quantiles.
  EXPECT_NO_THROW(s.num_quantiles(1));
  EXPECT_NO_THROW(s.num_quantiles(10));

  // Correct number of bins.
  EXPECT_NO_THROW(s.num_bins(1));
  EXPECT_NO_THROW(s.num_bins(10));

  // Correct seed.
  EXPECT_NO_THROW(s.seed(1));

  // Correct mission time.
  EXPECT_NO_THROW(s.mission_time(0));
  EXPECT_NO_THROW(s.mission_time(10));
  EXPECT_NO_THROW(s.mission_time(1e6));

  // Correct time step.
  EXPECT_NO_THROW(s.time_step(0));
  EXPECT_NO_THROW(s.time_step(10));
  EXPECT_NO_THROW(s.time_step(1e6));

  // Correct request for the SIL.
  EXPECT_NO_THROW(s.safety_integrity_levels(true));
  EXPECT_NO_THROW(s.safety_integrity_levels(false));
}

TEST(SettingsTest, SetupForPrimeImplicants) {
  Settings s;
  // Incorrect request for prime implicants.
  EXPECT_NO_THROW(s.algorithm("mocus"));
  EXPECT_THROW(s.prime_implicants(true), SettingsError);
  // Correct request for prime implicants.
  ASSERT_NO_THROW(s.algorithm("bdd"));
  ASSERT_NO_THROW(s.prime_implicants(true));
  // Prime implicants with quantitative approximations.
  EXPECT_NO_THROW(s.approximation("none"));
  EXPECT_THROW(s.approximation("rare-event"), SettingsError);
  EXPECT_THROW(s.approximation("mcub"), SettingsError);
}

}  // namespace scram::core::test
