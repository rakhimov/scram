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

#include "settings.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace core {
namespace test {

TEST(SettingsTest, IncorrectSetup) {
  Settings s;
  // Incorrect algorithm.
  EXPECT_THROW(s.algorithm("the-best"), InvalidArgument);
  // Incorrect approximation argument.
  EXPECT_THROW(s.approximation("approx"), InvalidArgument);
  // Incorrect limit order for products.
  EXPECT_THROW(s.limit_order(-1), InvalidArgument);
  // Incorrect cut-off probability.
  EXPECT_THROW(s.cut_off(-1), InvalidArgument);
  EXPECT_THROW(s.cut_off(10), InvalidArgument);
  // Incorrect number of trials.
  EXPECT_THROW(s.num_trials(-10), InvalidArgument);
  EXPECT_THROW(s.num_trials(0), InvalidArgument);
  // Incorrect number of quantiles.
  EXPECT_THROW(s.num_quantiles(-10), InvalidArgument);
  EXPECT_THROW(s.num_quantiles(0), InvalidArgument);
  // Incorrect number of bins.
  EXPECT_THROW(s.num_bins(-10), InvalidArgument);
  EXPECT_THROW(s.num_bins(0), InvalidArgument);
  // Incorrect seed.
  EXPECT_THROW(s.seed(-1), InvalidArgument);
  // Incorrect mission time.
  EXPECT_THROW(s.mission_time(-10), InvalidArgument);
  // Incorrect time step.
  EXPECT_THROW(s.time_step(-1), InvalidArgument);
  // The time step is not set for the SIL calculations.
  EXPECT_THROW(s.safety_integrity_levels(true), InvalidArgument);
  // Disable time step while the SIL is requested.
  EXPECT_NO_THROW(s.time_step(1));
  EXPECT_NO_THROW(s.safety_integrity_levels(true));
  EXPECT_THROW(s.time_step(0), InvalidArgument);
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
  EXPECT_THROW(s.prime_implicants(true), InvalidArgument);
  // Correct request for prime implicants.
  ASSERT_NO_THROW(s.algorithm("bdd"));
  ASSERT_NO_THROW(s.prime_implicants(true));
  // Prime implicants with quantitative approximations.
  EXPECT_NO_THROW(s.approximation("none"));
  EXPECT_THROW(s.approximation("rare-event"), InvalidArgument);
  EXPECT_THROW(s.approximation("mcub"), InvalidArgument);
}

}  // namespace test
}  // namespace core
}  // namespace scram
