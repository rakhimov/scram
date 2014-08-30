#include <gtest/gtest.h>

#include <unistd.h>

#include <vector>

#include "error.h"
#include "risk_analysis.h"

using namespace scram;

// Test correct tree inputs
TEST(RiskAnalysisInputTest, CorrectFTAInputs) {
  std::vector<std::string> correct_inputs;
  correct_inputs.push_back("./share/scram/input/fta/correct_tree_input.xml");
  // correct_inputs.push_back("./share/scram/input/fta/doubly_defined_basic.xml");

  RiskAnalysis* ran;

  std::vector<std::string>::iterator it;
  for (it = correct_inputs.begin(); it != correct_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_NO_THROW(ran->ProcessInput(*it)) << " Filename: " << *it;
    delete ran;
  }
  /// @todo Create include tests.
}

// Test correct probability inputs
TEST(RiskAnalysisInputTest, CorrectFTAProbability) {
  std::string input_correct = "./share/scram/input/fta/correct_tree_input_with_probs.xml";

  RiskAnalysis* ran;
  ran = new RiskAnalysis();
  EXPECT_NO_THROW(ran->ProcessInput(input_correct));  // Create the fault tree.
  delete ran;
}

// Test incorrect fault tree inputs
TEST(RiskAnalysisInputTest, IncorrectFTAInputs) {
  std::vector<std::string> ioerror_inputs;
  std::vector<std::string> incorrect_inputs;

  std::string dir = "./share/scram/input/fta/";

  // Access issues. IOErrors
  ioerror_inputs.push_back(dir + "nonexistent_file.xml");

  // Other issues.
  // incorrect_inputs.push_back("./share/scram/input/fta/basic_top_event.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_gate.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_house.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_basic.xml");
  incorrect_inputs.push_back(dir + "missing_basic_event_definition.xml");
  incorrect_inputs.push_back(dir + "missing_house_event_definition.xml");
  incorrect_inputs.push_back(dir + "missing_gate_definition.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/name_clash_inter.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/name_clash_primary.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/name_clash_top.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/non_existent_parent_primary.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/non_existent_parent_inter.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/unrecognized_parameter.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/unrecognized_type.xml");
  // incorrect_inputs.push_back("./share/scram/input/fta/vote_not_enough_children.xml");
  RiskAnalysis* ran;
  std::vector<std::string>::iterator it;
  for (it = ioerror_inputs.begin(); it != ioerror_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_THROW(ran->ProcessInput(*it), IOError) << " Filename:  " << *it;
    delete ran;
  }

  for (it = incorrect_inputs.begin(); it != incorrect_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_THROW(ran->ProcessInput(*it), ValidationError)
        << " Filename:  " << *it;
    delete ran;
  }
}
