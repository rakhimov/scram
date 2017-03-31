/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

namespace scram {
namespace core {
namespace test {

// Benchmark Tests for Baobab 2 fault tree from XFTA.
TEST_P(RiskAnalysisTest, Baobab2) {
  std::vector<std::string> input_files = {
      "./share/scram/input/Baobab/baobab2.xml",
      "./share/scram/input/Baobab/baobab2-basic-events.xml"};
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(4805, products().size());
  std::vector<int> distr = {0, 6, 121, 268, 630, 3780};
  EXPECT_EQ(distr, ProductDistribution());
}

}  // namespace test
}  // namespace core
}  // namespace scram
