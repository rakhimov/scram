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

#include "risk_analysis_tests.h"

#include <sstream>

#include "fault_tree.h"
#include "grapher.h"

namespace scram {
namespace test {

// Test Graphing Intructions
TEST_F(RiskAnalysisTest, GraphingInstructions) {
  Grapher gr = Grapher();
  std::stringstream out;
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile("./share/scram/input/fta/graphing.xml"));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    true, out));
}

TEST_F(RiskAnalysisTest, GraphingFlavoredTypes) {
  Grapher gr = Grapher();
  std::stringstream out;
  settings.probability_analysis(true);
  ASSERT_NO_THROW(
      ProcessInputFile("./share/scram/input/fta/flavored_types.xml"));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    true, out));
}

TEST_F(RiskAnalysisTest, GraphingNestedFormula) {
  Grapher gr = Grapher();
  std::stringstream out;
  ASSERT_NO_THROW(
      ProcessInputFile("./share/scram/input/fta/nested_formula.xml"));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    false, out));
}

TEST_F(RiskAnalysisTest, GraphingHouseUnity) {
  Grapher gr = Grapher();
  std::stringstream out;
  std::string tree_input = "./share/scram/input/core/unity.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    true, out));
}

TEST_F(RiskAnalysisTest, GraphingHouseNull) {
  Grapher gr = Grapher();
  std::stringstream out;
  std::string tree_input = "./share/scram/input/core/null.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    true, out));
}

}  // namespace test
}  // namespace scram
