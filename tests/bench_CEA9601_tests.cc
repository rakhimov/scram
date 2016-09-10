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

// Benchmark Tests for CEA9601 fault tree from XFTA.
#ifdef NDEBUG
TEST_F(RiskAnalysisTest, CEA9601_Test_BDD) {
  std::vector<std::string> input_files = {
      "./share/scram/input/CEA9601/CEA9601.xml",
      "./share/scram/input/CEA9601/CEA9601-basic-events.xml"};
  settings.limit_order(4).probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(analysis->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(54436, products().size());
  std::vector<int> distr = {0, 0, 1144, 53292};
  EXPECT_EQ(distr, ProductDistribution());

  EXPECT_NEAR(2.38155e-6, p_total(), 1e-10);
}

TEST_F(RiskAnalysisTest, CEA9601_Test_ZBDD) {
  std::vector<std::string> input_files = {
      "./share/scram/input/CEA9601/CEA9601.xml",
      "./share/scram/input/CEA9601/CEA9601-basic-events.xml"};
  settings.limit_order(3).algorithm("zbdd").probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(analysis->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(1144, products().size());
  std::vector<int> distr = {0, 0, 1144};
  EXPECT_EQ(distr, ProductDistribution());

  EXPECT_NEAR(3.316e-8, p_total(), 1e-10);
}
#endif

}  // namespace test
}  // namespace core
}  // namespace scram
