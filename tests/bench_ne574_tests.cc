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
namespace test {

// Benchmark Tests for an example fault tree given
// in NE574 Risk Analysis class at UW-Madison.
// Test Minimal cut sets and total probability.
TEST_P(RiskAnalysisTest, ne574) {
  std::string tree_input = "./share/scram/input/ne574/ne574.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  if (settings.approximation() == "rare-event") {
    EXPECT_DOUBLE_EQ(1, p_total());
  } else {
    EXPECT_NEAR(0.662208, p_total(), 1e-6);
  }

  std::set<std::set<std::string>> mcs = {{"c"}, {"d", "f"}, {"d", "g"},
                                         {"d", "b"}, {"h", "i", "f"},
                                         {"h", "i", "g"}, {"h", "i", "b"}};
  EXPECT_EQ(7, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

}  // namespace test
}  // namespace scram
