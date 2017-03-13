/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

// Benchmark Tests for HIPPS fault tree from XFTA.
// @todo Test Safety Integrity Level analysis.
TEST_P(RiskAnalysisTest, HIPPS) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/HIPPS/HIPPS.xml");
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.00162188, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.0016209, p_total(), 1e-5);
  }
  // Minimal cut set check.
  EXPECT_EQ(9, products().size());
  std::vector<int> distr = {6, 3};
  EXPECT_EQ(distr, ProductDistribution());
}

}  // namespace test
}  // namespace core
}  // namespace scram
