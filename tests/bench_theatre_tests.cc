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

namespace scram {
namespace test {

// Benchmark Tests for Theatre fault tree from OpenFTA.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, Theatre) {
  std::string tree_input = "./share/scram/input/Theatre/theatre.xml";
  std::string GEN_FAIL = "gen_fail";  // 2e-2
  std::string RELAY_FAIL = "relay_fail";  // 5e-2
  std::string MAINS_FAIL = "mains_fail";  // 3e-2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.00207, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(GEN_FAIL);
  cut_set.insert(MAINS_FAIL);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(MAINS_FAIL);
  cut_set.insert(RELAY_FAIL);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

}  // namespace test
}  // namespace scram
