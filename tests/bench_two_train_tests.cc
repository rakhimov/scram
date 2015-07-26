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
#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for an example fault tree with Two trains of Pumps and
// Valves.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, TwoTrain) {
  std::string tree_input = "./share/scram/input/TwoTrain/two_train.xml";
  std::string ValveOne = "valveone";  // 0.5
  std::string ValveTwo = "valvetwo";  // 0.5
  std::string PumpOne = "pumpone";  // 0.7
  std::string PumpTwo = "pumptwo";  // 0.7
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.7225, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(ValveOne);
  cut_set.insert(ValveTwo);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(PumpOne);
  cut_set.insert(PumpTwo);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(PumpOne);
  cut_set.insert(ValveTwo);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(ValveOne);
  cut_set.insert(PumpTwo);
  mcs.insert(cut_set);
  EXPECT_EQ(4, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
