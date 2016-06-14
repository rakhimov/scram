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

#include "initializer.h"

#include <vector>

#include <gtest/gtest.h>

#include "error.h"
#include "settings.h"

namespace scram {
namespace mef {
namespace test {

// Test if the XML is well formed.
TEST(InitializerTest, XMLFormatting) {
  Initializer* init = new Initializer(core::Settings());
  EXPECT_THROW(
      init->ProcessInputFiles({"./share/scram/input/xml_formatting_error.xml"}),
      ValidationError);
  delete init;
}

// Test the response for non-existent file.
TEST(InitializerTest, NonExistentFile) {
  // Access issues. IOErrors
  Initializer* init = new Initializer(core::Settings());
  EXPECT_THROW(
      init->ProcessInputFiles({"./share/scram/input/nonexistent_file.xml"}),
      IOError);
  delete init;
}

// Test if passing the same file twice causing an error.
TEST(InitializerTest, PassTheSameFileTwice) {
  std::string input_correct = "./share/scram/input/fta/correct_tree_input.xml";
  std::string the_same_path = "./share/.."  // Path obfuscation.
                              "/share/scram/input/fta/correct_tree_input.xml";
  Initializer* init = new Initializer(core::Settings());
  EXPECT_THROW(init->ProcessInputFiles({input_correct, the_same_path}),
               ValidationError);
  delete init;
}

// Test if the schema catches errors.
// This is trusted to XML libraries and the correctness of the RelaxNG schema,
// so the test is very basic calls.
TEST(InitializerTest, FailSchemaValidation) {
  Initializer* init = new Initializer(core::Settings());
  EXPECT_THROW(init->ProcessInputFiles({"./share/scram/input/schema_fail.xml"}),
               ValidationError);
  delete init;
}

// Unsupported operations.
TEST(InitializerTest, UnsupportedFeature) {
  std::string dir = "./share/scram/input/fta/";
  std::vector<std::string> incorrect_inputs = {"../unsupported_feature.xml",
                                               "../unsupported_gate.xml",
                                               "../unsupported_expression.xml"};
  for (const auto& input : incorrect_inputs) {
    Initializer* init = new Initializer(core::Settings());
    EXPECT_THROW(init->ProcessInputFiles({dir + input}), ValidationError)
        << " Filename:  " << input;
    delete init;
  }
}

TEST(InitializerTest, EmptyAttributeElementText) {
  std::string dir = "./share/scram/input/fta/";
  std::vector<std::string> incorrect_inputs = {"../empty_element.xml",
                                               "../empty_attribute.xml"};
  for (const auto& input : incorrect_inputs) {
    Initializer* init = new Initializer(core::Settings());
    EXPECT_THROW(init->ProcessInputFiles({dir + input}), ValidationError)
        << " Filename:  " << input;
    delete init;
  }
}

// Test correct inputs without probability information.
TEST(InitializerTest, CorrectFtaInputs) {
  std::string dir = "./share/scram/input/fta/";
  std::vector<std::string> correct_inputs = {
      "correct_tree_input.xml",
      "correct_formulas.xml",
      "component_definition.xml",
      "mixed_definitions.xml",
      "mixed_references.xml",
      "mixed_roles.xml",
      "model_data_mixed_definitions.xml",
      "two_trees.xml",
      "two_top_events.xml",
      "two_top_through_formula.xml",
      "labels_and_attributes.xml",
      "orphan_primary_event.xml",
      "very_long_mcs.xml",
      "unordered_structure.xml",
      "non_top_gate.xml",
      "unused_parameter.xml",
      "nested_formula.xml",
      "case_sensitivity.xml"};

  for (const auto& input : correct_inputs) {
    Initializer* init = new Initializer(core::Settings());
    EXPECT_NO_THROW(init->ProcessInputFiles({dir + input}))
        << " Filename: " << input;
    delete init;
  }
}

// Test correct inputs with probability information.
TEST(InitializerTest, CorrectProbabilityInputs) {
  std::string dir = "./share/scram/input/fta/";
  std::vector<std::string> correct_inputs = {
      "correct_tree_input_with_probs.xml",
      "trailing_spaces.xml",
      "correct_expressions.xml",
      "flavored_types.xml"};

  core::Settings settings;
  settings.probability_analysis(true);

  for (const auto& input : correct_inputs) {
    Initializer* init = new Initializer(settings);
    EXPECT_NO_THROW(init->ProcessInputFiles({dir + input}))
        << " Filename: " << input;
    delete init;
  }
}

// Test incorrect fault tree inputs
TEST(InitializerTest, IncorrectFtaInputs) {
  std::string dir = "./share/scram/input/fta/";

  std::vector<std::string> incorrect_inputs = {
      "int_overflow.xml",
      "invalid_probability.xml",
      "doubly_defined_gate.xml",
      "doubly_defined_house.xml",
      "doubly_defined_basic.xml",
      "doubly_defined_parameter.xml",
      "doubly_defined_ccf_group.xml",
      "doubly_defined_component.xml",
      "extra_ccf_level_beta_factor.xml",
      "missing_gate_definition.xml",
      "missing_ccf_level_number.xml",
      "missing_ccf_members.xml",
      "missing_arg_expression.xml",
      "undefined_event.xml",
      "undefined_basic_event.xml",
      "undefined_house_event.xml",
      "undefined_gate.xml",
      "undefined_parameter.xml",
      "reference_missing_fault_tree.xml",
      "reference_missing_component.xml",
      "wrong_parameter_unit.xml",
      "name_clash_two_trees.xml",
      "def_clash_basic_gate.xml",
      "def_clash_house_gate.xml",
      "def_clash_gate_primary.xml",
      "def_clash_basic_house.xml",
      "def_clash_house_basic.xml",
      "atleast_gate.xml",
      "cyclic_tree.xml",
      "cyclic_formula.xml",
      "cyclic_parameter.xml",
      "cyclic_expression.xml",
      "invalid_expression.xml",
      "repeated_child.xml",
      "alpha_ccf_level_error.xml",
      "beta_ccf_level_error.xml",
      "mgl_ccf_level_error.xml",
      "phi_ccf_wrong_sum.xml",
      "ccf_negative_factor.xml",
      "ccf_more_factors_than_needed.xml",
      "ccf_wrong_distribution.xml",
      "repeated_ccf_members.xml"};

  for (const auto& input : incorrect_inputs) {
    Initializer* init = new Initializer(core::Settings());
    EXPECT_THROW(init->ProcessInputFiles({dir + input}), ValidationError)
        << " Filename:  " << input;
    delete init;
  }
}

// Test failures triggered only in with probability analysis.
TEST(InitializerTest, IncorrectProbabilityInputs) {
  std::string dir = "./share/scram/input/fta/";
  std::vector<std::string> incorrect_inputs = {"missing_bool_constant.xml",
                                               "missing_expression.xml"};

  core::Settings settings;
  settings.probability_analysis(true);
  for (const auto& input : incorrect_inputs) {
    Initializer* init = new Initializer(settings);
    EXPECT_THROW(init->ProcessInputFiles({dir + input}), ValidationError)
        << " Filename:  " << input;
    delete init;
  }
}

// Test the case when a top event is not orphan.
// The top event of one fault tree
// can be a child of a gate of another fault tree.
TEST(InitializerTest, NonOrphanTopEvent) {
  std::string dir = "./share/scram/input/fta/";
  Initializer* init = new Initializer(core::Settings());
  EXPECT_NO_THROW(init->ProcessInputFiles({dir + "correct_tree_input.xml",
                                           dir + "second_fault_tree.xml"}));
  delete init;
}

}  // namespace test
}  // namespace mef
}  // namespace scram
