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

// Benchmark Tests for Small Tree fault tree from XFTA.
// This benchmark is for uncertainty analysis.
TEST_F(RiskAnalysisTest, SmallTree) {
  std::string tree_input = "./share/scram/input/SmallTree/SmallTree.xml";
  std::string e1 = "e1";
  std::string e2 = "e2";
  std::string e3 = "e3";
  std::string e4 = "e4";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.
  settings.uncertainty_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  cut_set.insert(e1);
  cut_set.insert(e2);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(e3);
  cut_set.insert(e4);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
  EXPECT_NEAR(0.02495, mean(), 5e-3);
  EXPECT_NEAR(0.023625, sigma(), 5e-3);
}
