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

#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

namespace scram {
namespace core {
namespace test {

// Benchmark Tests for the ThreeMotor fault tree from OpenFTA.
TEST_P(RiskAnalysisTest, ThreeMotor) {
  std::string tree_input = "./share/scram/input/ThreeMotor/three_motor.xml";
  std::set<std::set<std::string>> mcs;  // For expected min cut sets.
  std::string T3 = "T3";
  std::string S1 = "S1";
  std::string T4 = "T4";
  std::string T1 = "T1";
  std::string K2 = "K2";
  std::string T2 = "T2";
  std::string K5 = "K5";
  std::string K1 = "K1";
  std::string KT1inc = "KT1inc";
  std::string KT2inc = "KT2inc";
  std::string KT3inc = "KT3inc";
  std::string T1inc = "T1inc";
  std::string T2inc = "T2inc";
  std::string T3inc = "T3inc";
  std::string T4inc = "T4inc";

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.0212013, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.0211538, p_total(), 1e-5);
  }
  // Minimal cut set check.
  // Order 1
  mcs.insert({K5});

  // Order 2
  mcs.insert({S1, T2});
  mcs.insert({K1, T2});
  mcs.insert({T1inc, T2});

  // Order 3
  // Order 4
  mcs.insert({T2, T2inc, T3inc, T4inc});
  mcs.insert({KT3inc, T2, T2inc, T3inc});
  mcs.insert({KT2inc, T2, T2inc, T4inc});
  mcs.insert({KT2inc, KT3inc, T2, T2inc});
  mcs.insert({KT1inc, T2, T3inc, T4inc});
  mcs.insert({KT1inc, KT3inc, T2, T3inc});
  mcs.insert({KT1inc, KT2inc, T2, T4inc});
  mcs.insert({KT1inc, KT2inc, KT3inc, T2});

  EXPECT_EQ(12, products().size());
  EXPECT_EQ(mcs, products());
}

}  // namespace test
}  // namespace core
}  // namespace scram
