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

// Benchmark tests for Lift system from OpenFTA.
TEST_P(RiskAnalysisTest, Lift) {
  std::string tree_input = "./share/scram/input/Lift/lift.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_NEAR(1.19999e-5, p_total(), 1e-5);

  std::set<std::set<std::string>> mcs = {{"LMD_1"}, {"DPD_1"}, {"DM_1"},
                                         {"PS_1"}, {"DOD_1"}, {"DOD_2"},
                                         {"DMS_1"}, {"CP_1"}, {"DMS_2"},
                                         {"LMD_2"}, {"LPD_1"}, {"D_1"}};

  EXPECT_EQ(12, products().size());
  EXPECT_EQ(mcs, products());
}

}  // namespace test
}  // namespace core
}  // namespace scram
