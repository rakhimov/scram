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

#include <utility>

#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

namespace scram {
namespace core {
namespace test {

TEST_P(RiskAnalysisTest, AttackEventTree) {
  const char* tree_input = "./share/scram/input/EventTrees/attack.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(1, analysis->event_tree_results().size());
  auto& results = analysis->event_tree_results().front().sequences;
  ASSERT_EQ(2, results.size());
  EXPECT_NE(results.front().first.name(), results.back().first.name());
  EXPECT_EQ((std::set<std::string>{"AttackSucceeds", "AttackFails"}),
            (std::set<std::string>{results.front().first.name(),
                                  results.back().first.name()}));
  double p_success;
  double p_fail;
  std::tie(p_success, p_fail) = [&results]() -> std::pair<double, double> {
    if (results.front().first.name() == "AttackSucceeds") {
      return {results.front().second, results.back().second};
    }
    return {results.back().second, results.front().second};
  }();

  EXPECT_DOUBLE_EQ(0.772, p_success);
  EXPECT_DOUBLE_EQ(0.228, p_fail);
}

}  // namespace test
}  // namespace core
}  // namespace scram
