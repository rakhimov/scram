/*
 * Copyright (C) 2015 Olzhas Rakhimov
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

#include <gtest/gtest.h>

#include "bdd.h"
#include "fault_tree.h"

namespace scram {
namespace test {

TEST_F(RiskAnalysisTest, BddTest) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  GatePtr top_gate = fault_tree()->top_events().front();
  BooleanGraph* graph = new BooleanGraph(top_gate);
  Bdd* bdd = new Bdd(graph);
  bdd->Analyze();
  delete graph;
  delete bdd;
}

}  // namespace test
}  // namespace scram
