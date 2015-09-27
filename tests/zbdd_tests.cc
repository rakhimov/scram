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

#include "zbdd.h"
#include "fault_tree.h"
#include "preprocessor.h"

namespace scram {
namespace test {

TEST_F(RiskAnalysisTest, ZbddTest) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  GatePtr top_gate = fault_tree()->top_events().front();
  BooleanGraph* graph = new BooleanGraph(top_gate);
  Preprocessor* prep = new PreprocessorBdd(graph);
  prep->Run();
  Bdd* bdd = new Bdd(graph);
  Zbdd* zbdd = new Zbdd();
  zbdd->Analyze(bdd);
  EXPECT_EQ(4, zbdd->cut_sets().size());
  delete graph;
  delete bdd;
  delete prep;
  delete zbdd;
}

TEST_F(RiskAnalysisTest, ZbddChinese) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Chinese/chinese.xml");
  input_files.push_back("./share/scram/input/Chinese/chinese-basic-events.xml");
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  GatePtr top_gate = fault_tree()->top_events().front();
  BooleanGraph* graph = new BooleanGraph(top_gate);
  Preprocessor* prep = new PreprocessorBdd(graph);
  prep->Run();
  Bdd* bdd = new Bdd(graph);
  Zbdd* zbdd = new Zbdd();
  zbdd->Analyze(bdd);
  EXPECT_EQ(392, zbdd->cut_sets().size());
  delete graph;
  delete bdd;
  delete prep;
  delete zbdd;
}

TEST_F(RiskAnalysisTest, DISABLED_ZbddBaobab1) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Baobab/baobab1.xml");
  input_files.push_back("./share/scram/input/Baobab/baobab1-basic-events.xml");
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  GatePtr top_gate = fault_tree()->top_events().front();
  BooleanGraph* graph = new BooleanGraph(top_gate);
  Preprocessor* prep = new PreprocessorBdd(graph);
  prep->Run();
  Bdd* bdd = new Bdd(graph);
  Zbdd* zbdd = new Zbdd();
  zbdd->Analyze(bdd);
  EXPECT_EQ(392, zbdd->cut_sets().size());
  delete graph;
  delete prep;
  delete bdd;
  delete zbdd;
}

}  // namespace test
}  // namespace scram
