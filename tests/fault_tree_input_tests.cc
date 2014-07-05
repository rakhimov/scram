#include <gtest/gtest.h>

#include <unistd.h>

#include <vector>

#include "error.h"
#include "fault_tree.h"

using namespace scram;

// Test correct tree inputs
TEST(FaultTreeInputTest, CorrectFTAInputs) {
  std::vector<std::string> correct_inputs;
  correct_inputs.push_back("./input/fta/correct_tree_input.scramf");
  correct_inputs.push_back("./input/fta/doubly_defined_basic.scramf");
  correct_inputs.push_back("./input/fta/different_order.scramf");
  correct_inputs.push_back("./input/fta/inline_comments.scramf");
  correct_inputs.push_back("./input/fta/transfer_correct_top.scramf");

  RiskAnalysis* ran;

  std::vector<std::string>::iterator it;
  for (it = correct_inputs.begin(); it != correct_inputs.end(); ++it) {
    ran = new FaultTree("fta-default", false);
    EXPECT_NO_THROW(ran->ProcessInput(*it));
    delete ran;
  }

  // This test is written after detecting a bug for transfer trees.
  // Transfer input file path without current dir indicator "./"
  std::string clean_name = "transfer_correct_top.scramf";
  ASSERT_EQ(0, chdir("./input/fta/"));
  ran = new FaultTree("fta-default", false);
  EXPECT_NO_THROW(ran->ProcessInput(clean_name));
  ASSERT_EQ(0, chdir("../../"));  // Setting back the directory.
  delete ran;
}

// Test correct probability inputs
TEST(FaultTreeInputTest, CorrectFTAProbability) {
  std::string input_correct = "./input/fta/correct_tree_input.scramf";
  std::string prob_correct = "./input/fta/correct_prob_input.scramp";

  RiskAnalysis* ran = new FaultTree("fta-default", false);
  EXPECT_THROW(ran->PopulateProbabilities(prob_correct), Error);
  EXPECT_NO_THROW(ran->ProcessInput(input_correct));  // Create the fault tree.
  EXPECT_NO_THROW(ran->PopulateProbabilities(prob_correct));
}

// Test incorrect fault tree inputs
TEST(FaultTreeInputTest, IncorrectFTAInputs) {
  std::vector<std::string> incorrect_inputs;
  // Access issues.
  incorrect_inputs.push_back("./input/fta/nonexistent_file.scramf");
  // Formatting issues.
  incorrect_inputs.push_back("./input/fta/missing_opening_brace_at_start.scramf");
  incorrect_inputs.push_back("./input/fta/missing_opening_brace.scramf");
  incorrect_inputs.push_back("./input/fta/missing_closing_brace.scramf");
  incorrect_inputs.push_back("./input/fta/missing_closing_brace_at_end.scramf");
  // Other issues.
  incorrect_inputs.push_back("./input/fta/top_event_with_no_child.scramf");
  incorrect_inputs.push_back("./input/fta/basic_top_event.scramf");
  incorrect_inputs.push_back("./input/fta/doubly_defined_intermediate.scramf");
  incorrect_inputs.push_back("./input/fta/doubly_defined_top.scramf");
  incorrect_inputs.push_back("./input/fta/extra_parameter.scramf");
  incorrect_inputs.push_back("./input/fta/leaf_intermidiate_event.scramf");
  incorrect_inputs.push_back("./input/fta/missing_id.scramf");
  incorrect_inputs.push_back("./input/fta/missing_nodes.scramf");
  incorrect_inputs.push_back("./input/fta/missing_parameter.scramf");
  incorrect_inputs.push_back("./input/fta/missing_parent.scramf");
  incorrect_inputs.push_back("./input/fta/missing_type.scramf");
  incorrect_inputs.push_back("./input/fta/non_existent_parent.scramf");
  incorrect_inputs.push_back("./input/fta/unrecognized_parameter.scramf");
  incorrect_inputs.push_back("./input/fta/unrecognized_type.scramf");
  // Issues with transfer gates.
  incorrect_inputs.push_back("./input/fta/transfer_circular_self_top.scramf");
  incorrect_inputs.push_back("./input/fta/transfer_circular_top.scramf");
  incorrect_inputs.push_back("./input/fta/transfer_head_extra_nodes.scramf");
  incorrect_inputs.push_back("./input/fta/transfer_no_file.scramf");
  incorrect_inputs.push_back("./input/fta/transfer_wrong_parent.scramf");

  RiskAnalysis* ran;

  std::vector<std::string>::iterator it;
  for (it = incorrect_inputs.begin(); it != incorrect_inputs.end(); ++it) {
    ran = new FaultTree("fta-default", false);
    EXPECT_THROW(ran->ProcessInput(*it), scram::Error);
  }
}

// Test incorrect probability input
TEST(FaultTreeInputTest, IncorrectFTAProbability) {
  std::string correct_input = "./input/fta/correct_tree_input.scramf";
  std::vector<std::string> incorrect_prob;
  incorrect_prob.push_back("./input/fta/nonexistent_file.scramp");
  // Formatting issues.
  incorrect_prob.push_back("./input/fta/missing_opening_brace_at_start.scramp");
  incorrect_prob.push_back("./input/fta/missing_opening_brace.scramp");
  incorrect_prob.push_back("./input/fta/missing_closing_brace.scramp");
  incorrect_prob.push_back("./input/fta/missing_closing_brace_at_end.scramp");
  // Other issues.
  incorrect_prob.push_back("./input/fta/doubly_defined_prob.scramp");
  incorrect_prob.push_back("./input/fta/huge_prob.scramp");
  incorrect_prob.push_back("./input/fta/missing_basic_event.scramp");
  incorrect_prob.push_back("./input/fta/string_prob.scramp");
  incorrect_prob.push_back("./input/fta/negative_prob.scramp");

  RiskAnalysis* ran;
  std::vector<std::string>::iterator it;
  for (it = incorrect_prob.begin(); it != incorrect_prob.end(); ++it) {
    ran = new FaultTree("fta-default", false);
    EXPECT_NO_THROW(ran->ProcessInput(correct_input));
    EXPECT_THROW(ran->PopulateProbabilities(*it), scram::Error);
  }
}
