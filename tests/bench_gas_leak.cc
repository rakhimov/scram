/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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

#include "risk_analysis_tests.h"

namespace scram::core::test {

TEST_F(RiskAnalysisTest, GasLeakReactive) {
  const char* tree_input = "input/EventTrees/gas_leak/gas_leak_reactive.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles({tree_input}));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(1, analysis->event_tree_results().size());
  std::map<std::string, double> expected = {
      {"S1", 0.81044}, {"S2", 0.04479}, {"S3", 0.04265}, {"S4", 2.36e-3},
      {"S5", 0.04265}, {"S6", 2.36e-3}, {"S7", 4.5e-3},  {"S8", 0.05025}};
  const auto& results = sequences();
  ASSERT_EQ(8, results.size());
  for (const auto& result : expected) {
    INFO("seq: " + result.first);
    ASSERT_TRUE(results.count(result.first));
    EXPECT_NEAR(result.second, results.at(result.first), 1e-5);
  }
}

/// @todo Expand
TEST_F(RiskAnalysisTest, GasLeak) {
  settings.probability_analysis(true);
  ASSERT_NO_THROW(
      ProcessInputFiles({"input/EventTrees/gas_leak/gas_leak_reactive.xml",
                         "input/EventTrees/gas_leak/gas_leak.xml"}));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(2, analysis->event_tree_results().size());
}

}  // namespace scram::core::test
