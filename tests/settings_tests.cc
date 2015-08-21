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

#include "settings.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace test {

TEST(SettingsTest, IncorrectSetup) {
  Settings s;
  // Incorrect approximation argument.
  ASSERT_THROW(s.approx("approx"), InvalidArgument);
  // Incorrect limit order for minimal cut sets.
  ASSERT_THROW(s.limit_order(-1), InvalidArgument);
  // Incorrect number of series in the probability equation.
  ASSERT_THROW(s.num_sums(-1), InvalidArgument);
  ASSERT_THROW(s.num_sums(0), InvalidArgument);
  // Incorrect cut-off probability.
  ASSERT_THROW(s.cut_off(-1), InvalidArgument);
  ASSERT_THROW(s.cut_off(10), InvalidArgument);
  // Incorrect number of trials.
  ASSERT_THROW(s.num_trials(-10), InvalidArgument);
  ASSERT_THROW(s.num_trials(0), InvalidArgument);
  // Incorrect number of quantiles.
  ASSERT_THROW(s.num_quantiles(-10), InvalidArgument);
  ASSERT_THROW(s.num_quantiles(0), InvalidArgument);
  // Incorrect number of bins.
  ASSERT_THROW(s.num_bins(-10), InvalidArgument);
  ASSERT_THROW(s.num_bins(0), InvalidArgument);
  // Incorrect seed.
  ASSERT_THROW(s.seed(-1), InvalidArgument);
  // Incorrect mission time.
  ASSERT_THROW(s.mission_time(-10), InvalidArgument);
}

}  // namespace test
}  // namespace scram
