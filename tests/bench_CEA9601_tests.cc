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

// Benchmark Tests for CEA9601 fault tree from XFTA.
// Test Minimal cut sets.
// This test is disabled
// becuase of the current handling of non-coherent trees
// with minimal cut sets instead of prime implicants.
/// @todo Enable when prime implicants are introduced.
TEST_F(RiskAnalysisTest, DISABLED_CEA9601_Test) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/CEA9601/CEA9601.xml");
  input_files.push_back("./share/scram/input/CEA9601/CEA9601-basic-events.xml");
  settings.limit_order(4);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(2732, min_cut_sets().size());
  std::vector<int> distr(5, 0);
  distr[3] = 858;
  distr[4] = 1874;
  EXPECT_EQ(distr, McsDistribution());
}

}  // namespace test
}  // namespace scram
