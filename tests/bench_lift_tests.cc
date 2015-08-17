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

// Benchmark tests for Lift system from OpenFTA
TEST_F(RiskAnalysisTest, Lift) {
  std::string tree_input = "./share/scram/input/Lift/lift.xml";
  std::vector<std::string> events;
  events.push_back("lmd_1");
  events.push_back("dpd_1");
  events.push_back("dm_1");
  events.push_back("ps_1");
  events.push_back("dod_1");
  events.push_back("dod_2");
  events.push_back("dms_1");
  events.push_back("cp_1");
  events.push_back("dms_2");
  events.push_back("lmd_2");
  events.push_back("lpd_1");
  events.push_back("d_1");

  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(1.19999e-5, p_total(), 1e-5);
  // Minimal cut set check.
  std::vector<std::string>::iterator it;
  for (it = events.begin(); it != events.end(); ++it) {
    cut_set.insert(*it);
    mcs.insert(cut_set);
    cut_set.clear();
  }
  EXPECT_EQ(12, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

}  // namespace test
}  // namespace scram
