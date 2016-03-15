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
namespace test {

// Benchmark Tests for Chinese fault tree from XFTA.
/// @todo Test importance factors.
TEST_P(RiskAnalysisTest, ChineseTree) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Chinese/chinese.xml");
  input_files.push_back("./share/scram/input/Chinese/chinese-basic-events.xml");
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  if (settings.approximation() == "rare-event") {
    EXPECT_NEAR(0.004804, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.0045691, p_total(), 1e-5);
  }
  // Minimal cut set check.
  EXPECT_EQ(392, products().size());
  std::vector<int> distr = {0, 12, 0, 24, 188, 168};
  EXPECT_EQ(distr, ProductDistribution());
}

}  // namespace test
}  // namespace scram
