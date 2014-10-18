#include <gtest/gtest.h>

#include <unistd.h>

#include <vector>

#include "error.h"
#include "risk_analysis.h"

using namespace scram;

// Test if the xml is well formed.
TEST(RiskAnalysisInputTest, XMLFormatting) {
  std::string input_incorrect= "./share/scram/input/xml_formatting_error.xml";
  RiskAnalysis* ran;
  ran = new RiskAnalysis();
  EXPECT_THROW(ran->ProcessInput(input_incorrect), ValidationError);
  delete ran;
}

// Test if the schema catches errors.
// This is trusted to XML libraries and the correctness of the RelaxNG schema,
// so the test is very basic calls.
TEST(RiskAnalysisInputTest, FailSchemaValidation) {
  std::string input_incorrect= "./share/scram/input/schema_fail.xml";
  RiskAnalysis* ran;
  ran = new RiskAnalysis();
  EXPECT_THROW(ran->ProcessInput(input_incorrect), ValidationError);
  delete ran;
}

// Unsupported operations.
TEST(RiskAnalysisInputTest, UnsupportedFeature) {
  std::vector<std::string> incorrect_inputs;

  std::string dir = "./share/scram/input/fta/";
  incorrect_inputs.push_back(dir + "../unsupported_feature.xml");
  incorrect_inputs.push_back(dir + "unsupported_gate.xml");
  incorrect_inputs.push_back(dir + "unsupported_expression.xml");
  RiskAnalysis* ran;
  std::vector<std::string>::iterator it;
  for (it = incorrect_inputs.begin(); it != incorrect_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_THROW(ran->ProcessInput(*it), ValidationError)
        << " Filename:  " << *it;
    delete ran;
  }
}

// Test correct tree inputs
TEST(RiskAnalysisInputTest, CorrectFTAInputs) {
  std::vector<std::string> correct_inputs;
  std::string dir = "./share/scram/input/fta/";
  correct_inputs.push_back(dir + "correct_tree_input.xml");
  correct_inputs.push_back(dir + "mixed_definitions.xml");
  correct_inputs.push_back(dir + "model_data_mixed_definitions.xml");
  correct_inputs.push_back(dir + "trailing_spaces.xml");
  correct_inputs.push_back(dir + "two_trees.xml");
  correct_inputs.push_back(dir + "labels_and_attributes.xml");
  correct_inputs.push_back(dir + "orphan_primary_event.xml");
  correct_inputs.push_back(dir + "correct_expressions.xml");
  correct_inputs.push_back(dir + "flavored_types.xml");

  RiskAnalysis* ran;

  std::vector<std::string>::iterator it;
  for (it = correct_inputs.begin(); it != correct_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_NO_THROW(ran->ProcessInput(*it)) << " Filename: " << *it;
    EXPECT_NO_THROW(ran->Report("cli")) << " Filename: " << *it;
    delete ran;
  }
  /// @todo Create include tests.
}

// Test correct probability inputs
TEST(RiskAnalysisInputTest, CorrectFTAProbability) {
  std::string input_correct =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";

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
  incorrect_inputs.push_back(dir + "doubly_defined_gate.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_house.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_basic.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_parameter.xml");
  incorrect_inputs.push_back(dir + "missing_event_definition.xml");
  incorrect_inputs.push_back(dir + "missing_basic_event_definition.xml");
  incorrect_inputs.push_back(dir + "missing_house_event_definition.xml");
  incorrect_inputs.push_back(dir + "missing_expression.xml");
  incorrect_inputs.push_back(dir + "missing_bool_constant.xml");
  incorrect_inputs.push_back(dir + "missing_parameter.xml");
  incorrect_inputs.push_back(dir + "missing_gate_definition.xml");
  incorrect_inputs.push_back(dir + "name_clash_basic_gate.xml");
  incorrect_inputs.push_back(dir + "name_clash_house_gate.xml");
  incorrect_inputs.push_back(dir + "name_clash_gate_primary.xml");
  incorrect_inputs.push_back(dir + "name_clash_basic_house.xml");
  incorrect_inputs.push_back(dir + "name_clash_house_basic.xml");
  incorrect_inputs.push_back(dir + "name_clash_two_trees.xml");
  incorrect_inputs.push_back(dir + "def_clash_basic_gate.xml");
  incorrect_inputs.push_back(dir + "def_clash_house_gate.xml");
  incorrect_inputs.push_back(dir + "def_clash_gate_primary.xml");
  incorrect_inputs.push_back(dir + "def_clash_basic_house.xml");
  incorrect_inputs.push_back(dir + "def_clash_house_basic.xml");
  incorrect_inputs.push_back(dir + "def_name_house_basic.xml");
  incorrect_inputs.push_back(dir + "def_name_basic_house.xml");
  incorrect_inputs.push_back(dir + "atleast_gate.xml");
  incorrect_inputs.push_back(dir + "unordered_structure.xml");
  incorrect_inputs.push_back(dir + "dangling_gate.xml");
  incorrect_inputs.push_back(dir + "non_top_gate.xml");
  incorrect_inputs.push_back(dir + "cyclic_tree.xml");
  incorrect_inputs.push_back(dir + "cyclic_parameter.xml");
  incorrect_inputs.push_back(dir + "invalid_probability.xml");
  incorrect_inputs.push_back(dir + "invalid_expression.xml");
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
