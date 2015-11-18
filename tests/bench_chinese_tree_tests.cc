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

// Benchmark Tests for Chinese fault tree from XFTA.
// Test Minimal cut sets and probability.
/// @todo Test importance factors.
TEST_F(RiskAnalysisTest, ChineseTree) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Chinese/chinese.xml");
  input_files.push_back("./share/scram/input/Chinese/chinese-basic-events.xml");
  settings.probability_analysis(true).limit_order(6);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.0045691, p_total(), 1e-5);
  // Minimal cut set check.
  EXPECT_EQ(392, min_cut_sets().size());
  std::vector<int> distr = {0, 0, 12, 0, 24, 188, 168};
  EXPECT_EQ(distr, McsDistribution());
}

TEST_F(RiskAnalysisTest, ChineseTreeBdd) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Chinese/chinese.xml");
  input_files.push_back("./share/scram/input/Chinese/chinese-basic-events.xml");
  settings.probability_analysis(true).limit_order(6).algorithm("bdd");
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.0045691, p_total(), 1e-5);
  // Minimal cut set check.
  EXPECT_EQ(392, min_cut_sets().size());
  std::vector<int> distr = {0, 0, 12, 0, 24, 188, 168};
  EXPECT_EQ(distr, McsDistribution());
}

TEST_F(RiskAnalysisTest, ChineseTreeZbdd) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Chinese/chinese.xml");
  input_files.push_back("./share/scram/input/Chinese/chinese-basic-events.xml");
  settings.probability_analysis(true).limit_order(6).algorithm("zbdd");
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.0045691, p_total(), 1e-5);
  // Minimal cut set check.
  EXPECT_EQ(392, min_cut_sets().size());
  std::vector<int> distr = {0, 0, 12, 0, 24, 188, 168};
  EXPECT_EQ(distr, McsDistribution());
}

}  // namespace test
}  // namespace scram
