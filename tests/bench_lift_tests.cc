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
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(1.19999e-5, p_total(), 1e-5);

  std::set< std::set<std::string> > mcs = {{"lmd_1"}, {"dpd_1"}, {"dm_1"},
                                           {"ps_1"}, {"dod_1"}, {"dod_2"},
                                           {"dms_1"}, {"cp_1"}, {"dms_2"},
                                           {"lmd_2"}, {"lpd_1"}, {"d_1"}};

  EXPECT_EQ(12, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

}  // namespace test
}  // namespace scram
