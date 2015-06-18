#include <unistd.h>

#include <vector>

#include <gtest/gtest.h>

#include "error.h"
#include "risk_analysis.h"
#include "settings.h"

using namespace scram;

// Test if the XML is well formed.
TEST(RiskAnalysisInputTest, XMLFormatting) {
  std::string input_incorrect = "./share/scram/input/xml_formatting_error.xml";
  RiskAnalysis* ran;
  ran = new RiskAnalysis();
  EXPECT_THROW(ran->ProcessInput(input_incorrect), ValidationError);
  delete ran;
}

// Test if passing the same file twice causing an error.
TEST(RiskAnalysisInputTest, PassTheSameFileTwice) {
  std::string input_correct = "./share/scram/input/fta/correct_tree_input.xml";
  std::string the_same_path = "./share/.."
                              "/share/scram/input/fta/correct_tree_input.xml";
  RiskAnalysis* ran;
  ran = new RiskAnalysis();
  EXPECT_NO_THROW(ran->ProcessInput(input_correct));
  EXPECT_THROW(ran->ProcessInput(the_same_path), ValidationError);
  delete ran;
}

// Test if the schema catches errors.
// This is trusted to XML libraries and the correctness of the RelaxNG schema,
// so the test is very basic calls.
TEST(RiskAnalysisInputTest, FailSchemaValidation) {
  std::string input_incorrect = "./share/scram/input/schema_fail.xml";
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
  incorrect_inputs.push_back(dir + "../unsupported_gate.xml");
  incorrect_inputs.push_back(dir + "../unsupported_expression.xml");
  RiskAnalysis* ran;
  std::vector<std::string>::iterator it;
  for (it = incorrect_inputs.begin(); it != incorrect_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_THROW(ran->ProcessInput(*it), ValidationError)
        << " Filename:  " << *it;
    delete ran;
  }
}

// Test correct inputs without probability information.
TEST(RiskAnalysisInputTest, CorrectFTAInputs) {
  std::vector<std::string> correct_inputs;
  std::string dir = "./share/scram/input/fta/";
  correct_inputs.push_back(dir + "correct_tree_input.xml");
  correct_inputs.push_back(dir + "correct_formulas.xml");
  correct_inputs.push_back(dir + "mixed_definitions.xml");
  correct_inputs.push_back(dir + "model_data_mixed_definitions.xml");
  correct_inputs.push_back(dir + "two_trees.xml");
  correct_inputs.push_back(dir + "two_top_events.xml");
  correct_inputs.push_back(dir + "two_top_through_formula.xml");
  correct_inputs.push_back(dir + "labels_and_attributes.xml");
  correct_inputs.push_back(dir + "orphan_primary_event.xml");
  correct_inputs.push_back(dir + "very_long_mcs.xml");
  correct_inputs.push_back(dir + "unordered_structure.xml");
  correct_inputs.push_back(dir + "non_top_gate.xml");
  correct_inputs.push_back(dir + "unused_parameter.xml");

  RiskAnalysis* ran;

  std::vector<std::string>::iterator it;
  for (it = correct_inputs.begin(); it != correct_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_NO_THROW(ran->ProcessInput(*it)) << " Filename: " << *it;
    delete ran;
  }
}

// Test correct inputs with probability information.
TEST(RiskAnalysisInputTest, CorrectProbInputs) {
  std::vector<std::string> correct_inputs;
  std::string dir = "./share/scram/input/fta/";
  correct_inputs.push_back(dir + "correct_tree_input_with_probs.xml");
  correct_inputs.push_back(dir + "trailing_spaces.xml");
  correct_inputs.push_back(dir + "correct_expressions.xml");
  correct_inputs.push_back(dir + "flavored_types.xml");

  RiskAnalysis* ran;
  Settings settings;
  settings.probability_analysis(true);

  std::vector<std::string>::iterator it;
  for (it = correct_inputs.begin(); it != correct_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    ran->AddSettings(settings);
    EXPECT_NO_THROW(ran->ProcessInput(*it)) << " Filename: " << *it;
    delete ran;
  }
}

// Test incorrect fault tree inputs
TEST(RiskAnalysisInputTest, IncorrectFTAInputs) {
  std::vector<std::string> incorrect_inputs;

  std::string dir = "./share/scram/input/fta/";

  // Access issues. IOErrors
  std::string ioerror_input = dir + "nonexistent_file.xml";
  RiskAnalysis* ran = new RiskAnalysis();
  EXPECT_THROW(ran->ProcessInput(ioerror_input), IOError)
      << " Filename:  " << ioerror_input;
  delete ran;

  // Other issues.
  incorrect_inputs.push_back(dir + "doubly_defined_gate.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_house.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_basic.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_parameter.xml");
  incorrect_inputs.push_back(dir + "doubly_defined_ccf_group.xml");
  incorrect_inputs.push_back(dir + "extra_ccf_level_beta_factor.xml");
  incorrect_inputs.push_back(dir + "missing_gate_definition.xml");
  incorrect_inputs.push_back(dir + "missing_ccf_level_number.xml");
  incorrect_inputs.push_back(dir + "missing_ccf_members.xml");
  incorrect_inputs.push_back(dir + "undefined_event.xml");
  incorrect_inputs.push_back(dir + "undefined_basic_event.xml");
  incorrect_inputs.push_back(dir + "undefined_house_event.xml");
  incorrect_inputs.push_back(dir + "undefined_gate.xml");
  incorrect_inputs.push_back(dir + "undefined_parameter.xml");
  incorrect_inputs.push_back(dir + "wrong_parameter_unit.xml");
  incorrect_inputs.push_back(dir + "name_clash_two_trees.xml");
  incorrect_inputs.push_back(dir + "def_clash_basic_gate.xml");
  incorrect_inputs.push_back(dir + "def_clash_house_gate.xml");
  incorrect_inputs.push_back(dir + "def_clash_gate_primary.xml");
  incorrect_inputs.push_back(dir + "def_clash_basic_house.xml");
  incorrect_inputs.push_back(dir + "def_clash_house_basic.xml");
  incorrect_inputs.push_back(dir + "atleast_gate.xml");
  incorrect_inputs.push_back(dir + "cyclic_tree.xml");
  incorrect_inputs.push_back(dir + "cyclic_formula.xml");
  incorrect_inputs.push_back(dir + "cyclic_parameter.xml");
  incorrect_inputs.push_back(dir + "cyclic_expression.xml");
  incorrect_inputs.push_back(dir + "invalid_expression.xml");
  incorrect_inputs.push_back(dir + "repeated_child.xml");
  incorrect_inputs.push_back(dir + "alpha_ccf_level_error.xml");
  incorrect_inputs.push_back(dir + "beta_ccf_level_error.xml");
  incorrect_inputs.push_back(dir + "mgl_ccf_level_error.xml");
  incorrect_inputs.push_back(dir + "phi_ccf_wrong_sum.xml");
  incorrect_inputs.push_back(dir + "ccf_negative_factor.xml");
  incorrect_inputs.push_back(dir + "ccf_more_factors_than_needed.xml");
  incorrect_inputs.push_back(dir + "repeated_ccf_members.xml");

  std::vector<std::string>::iterator it;
  for (it = incorrect_inputs.begin(); it != incorrect_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    EXPECT_THROW(ran->ProcessInput(*it), ValidationError)
        << " Filename:  " << *it;
    delete ran;
  }
}

TEST(RiskAnalysisInputTest, IncorrectProbInputs) {
  std::vector<std::string> incorrect_inputs;
  std::string dir = "./share/scram/input/fta/";
  incorrect_inputs.push_back(dir + "invalid_probability.xml");
  incorrect_inputs.push_back(dir + "missing_bool_constant.xml");
  incorrect_inputs.push_back(dir + "missing_expression.xml");
  incorrect_inputs.push_back(dir + "ccf_wrong_distribution.xml");

  RiskAnalysis* ran;
  Settings settings;
  settings.probability_analysis(true);
  std::vector<std::string>::iterator it;
  for (it = incorrect_inputs.begin(); it != incorrect_inputs.end(); ++it) {
    ran = new RiskAnalysis();
    ran->AddSettings(settings);
    EXPECT_THROW(ran->ProcessInput(*it), ValidationError)
        << " Filename:  " << *it;
    delete ran;
  }
}

// Test the case when a top event is not orphan. The top event of one fault
// tree can be a child of a gate of another fault tree.
TEST(RiskAnalysisInputTest, NonOrphanTopEvent) {
  std::string dir = "./share/scram/input/fta/";
  std::vector<std::string> input_files;

  input_files.push_back(dir + "correct_tree_input.xml");
  input_files.push_back(dir + "second_fault_tree.xml");

  RiskAnalysis* ran = new RiskAnalysis();
  EXPECT_NO_THROW(ran->ProcessInputFiles(input_files));
  delete ran;
}
