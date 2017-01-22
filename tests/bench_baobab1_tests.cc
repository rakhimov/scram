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

// Benchmark Tests for Baobab 1 fault tree from XFTA.
#ifdef NDEBUG
TEST_P(RiskAnalysisTest, Baobab1) {
#else
TEST_F(RiskAnalysisTest, Baobab1) {
  settings.algorithm("bdd");
#endif
  std::vector<std::string> input_files = {
      "./share/scram/input/Baobab/baobab1.xml",
      "./share/scram/input/Baobab/baobab1-basic-events.xml"};
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(1.6815e-6, p_total(), 1e-8);
  } else {  // Probability with BDD.
    EXPECT_NEAR(1.2823e-6, p_total(), 1e-8);
  }
  EXPECT_EQ(46188, products().size());
  std::vector<int> distr = {0,     1,    1,     70,   400, 2212,
                            14748, 8460, 10624, 6600, 3072};
  EXPECT_EQ(distr, ProductDistribution());
}

TEST_P(RiskAnalysisTest, Baobab1L8) {
  std::vector<std::string> input_files = {
      "./share/scram/input/Baobab/baobab1.xml",
      "./share/scram/input/Baobab/baobab1-basic-events.xml"};
  settings.limit_order(8);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(25892, products().size());
  std::vector<int> distr = {0, 1, 1, 70, 400, 2212, 14748, 8460};
  EXPECT_EQ(distr, ProductDistribution());
}

}  // namespace test
}  // namespace core
}  // namespace scram
