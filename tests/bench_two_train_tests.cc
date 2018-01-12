/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

// Benchmark Tests for an example fault tree with
// Two trains of Pumps and Valves.
TEST_P(RiskAnalysisTest, TwoTrain) {
  std::string tree_input = "input/TwoTrain/two_train.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles({tree_input}));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(1, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.7225, p_total());
  }
  std::set<std::set<std::string>> mcs = {{"ValveOne", "ValveTwo"},
                                         {"ValveOne", "PumpTwo"},
                                         {"ValveTwo", "PumpOne"},
                                         {"PumpOne", "PumpTwo"}};
  EXPECT_EQ(4, products().size());
  EXPECT_EQ(mcs, products());
}

TEST_P(RiskAnalysisTest, TwoTrainUnityEventTree) {
  std::string dir = "input/TwoTrain/";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(
      ProcessInputFiles({dir + "two_train.xml", dir + "event_tree.xml"}));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(1, analysis->event_tree_results().size());
  const auto& results = sequences();
  ASSERT_EQ(1, results.size());
  EXPECT_EQ((std::map<std::string, double>{{"S", 1}}), results);
}

TEST_P(RiskAnalysisTest, TwoTrainSubstitutions) {
  if (settings.prime_implicants())
    return;  /// @todo Solve the test for prime implicants.

  std::string dir = "input/TwoTrain/";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFiles({dir + "substitutions.xml"}));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(1, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.329175, p_total());
  }
  std::set<std::set<std::string>> mcs = {
      {"ValveOne", "PumpTwo"},
      {"ValveTwo", "PumpOne"},
      {"PumpOne", "PumpTwo", "HotBackupPump", "ColdBackupPump"}};
  EXPECT_EQ(3, products().size());
  EXPECT_EQ(mcs, products());
}

TEST_P(RiskAnalysisTest, TwoTrainNonDeclarativeSubstitutions) {
  if (settings.prime_implicants())
    return;  /// @todo Solve the test for prime implicants.

  std::string dir = "input/TwoTrain/";
  settings.probability_analysis(true);
  settings.approximation(core::Approximation::kRareEvent);
  ASSERT_NO_THROW(
      ProcessInputFiles({dir + "nondeclarative_substitutions.xml"}));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());
  std::set<std::set<std::string>> mcs = {{"ValveOne", "PumpTwo"},
                                         {"ValveTwo", "PumpOne"},
                                         {"ValveOne", "ValveThree"},
                                         {"HotBackupPump"}};
  EXPECT_EQ(4, products().size());
  EXPECT_EQ(mcs, products());
}

}  // namespace scram::core::test
