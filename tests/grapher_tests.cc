#include "risk_analysis_tests.h"

#include <sstream>

#include "fault_tree.h"
#include "grapher.h"

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

TEST_F(RiskAnalysisTest, GraphingHOUSE_UNITY) {
  Grapher gr = Grapher();
  std::stringstream out;
  std::string tree_input = "./share/scram/input/core/unity.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    true, out));
}

TEST_F(RiskAnalysisTest, GraphingHOUSE_NULL) {
  Grapher gr = Grapher();
  std::stringstream out;
  std::string tree_input = "./share/scram/input/core/null.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(gr.GraphFaultTree(fault_tree()->top_events().front(),
                                    true, out));
}
