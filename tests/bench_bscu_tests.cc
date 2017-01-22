/*
 * Copyright (C) 2016-2017 Olzhas Rakhimov
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

// Benchmark Tests for BSCU fault tree from XFTA.
// This benchmark is for uncertainty analysis.
TEST_P(RiskAnalysisTest, BSCU) {
  std::string tree_input = "./share/scram/input/BSCU/BSCU.xml";
  settings.uncertainty_analysis(true);
  settings.num_trials(10000);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {
      {"SwitchStuckInIntermediatePosition"},
      {"ValidityMonitorFailure"},
      {"LossOfSystem1PowerSupply", "LossOfSystem2PowerSupply"},
      {"LossOfSystem1PowerSupply", "SwitchStuckInPosition1"},
      {"LossOfSystem1PowerSupply", "System2ElectronicFailure"},
      {"LossOfSystem2PowerSupply", "SwitchStuckInPosition2"},
      {"LossOfSystem2PowerSupply", "System1ElectronicFailure"},
      {"SwitchStuckInPosition1", "System1ElectronicFailure"},
      {"SwitchStuckInPosition2", "System2ElectronicFailure"},
      {"System1ElectronicFailure", "System2ElectronicFailure"}};

  EXPECT_EQ(10, products().size());
  EXPECT_EQ(mcs, products());

  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.135372, p_total(), 1e-4);
    EXPECT_NEAR(0.137, mean(), 5e-3);
    EXPECT_NEAR(0.217, sigma(), 5e-3);
  } else {
    EXPECT_NEAR(0.1124087, p_total(), 1e-4);
    EXPECT_NEAR(0.117, mean(), 5e-3);
    EXPECT_NEAR(0.183, sigma(), 5e-3);
  }
}


}  // namespace test
}  // namespace core
}  // namespace scram
