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

#include "uncertainty_analysis.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace test {

TEST(UncertaintyAnalysisTest, Constructor) {
  ASSERT_NO_THROW(UncertaintyAnalysis(1));
  // Incorrect number of series in the probability equation.
  ASSERT_THROW(UncertaintyAnalysis(-1), InvalidArgument);
  // Invalid cut-off probability.
  ASSERT_NO_THROW(UncertaintyAnalysis(1, 1));
  ASSERT_THROW(UncertaintyAnalysis(1, -1), InvalidArgument);
  // Invalid number of trials.
  ASSERT_NO_THROW(UncertaintyAnalysis(1, 1, 100));
  ASSERT_THROW(UncertaintyAnalysis(1, 1, -1), InvalidArgument);
}

}  // namespace test
}  // namespace scram
