#include "fault_tree_tests.h"

using namespace scram;

// Test Input Processing
// Note that there are tests specificly for correct and incorrect inputs
// in fault_tree_input_tests.cc, so this test only concerned with actual changes
// after processing the input.
TEST_F(FaultTreeTest, ProcessInput) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  FaultTree* fta = new FaultTree("fta-default", false);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
}

// Test Probability Assignment
TEST_F(FaultTreeTest, PopulateProbabilities) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  FaultTree* fta = new FaultTree("fta-default", false);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
}

// Test Graphing Intructions
TEST_F(FaultTreeTest, GraphingInstructions) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  FaultTree* fta = new FaultTree("fta-default", false);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->GraphingInstructions());
}

// Test Analysis
TEST_F(FaultTreeTest, Analyze) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  FaultTree* fta = new FaultTree("fta-default", false);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
}

// Test Reporting capabilities
TEST_F(FaultTreeTest, Report) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  FaultTree* fta = new FaultTree("fta-default", false);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
}
