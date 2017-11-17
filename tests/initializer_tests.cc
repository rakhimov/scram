/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

#include <gtest/gtest.h>

#include "error.h"
#include "settings.h"

namespace scram {
namespace mef {
namespace test {

// Test if the XML is well formed.
TEST(InitializerTest, XMLFormatting) {
  EXPECT_THROW(Initializer({"./share/scram/input/xml_formatting_error.xml"},
                           core::Settings()),
               xml::ParseError);
}

// XML custom namespace errors.
TEST(InitializerTest, XMLNameSpace) {
  EXPECT_THROW(Initializer({"./share/scram/input/undefined_xmlns.xml"},
                           core::Settings()),
               xml::ParseError);
  EXPECT_THROW(Initializer({"./share/scram/input/custom_xmlns.xml"},
                           core::Settings()),
               xml::ValidityError);
}

// Test the response for non-existent file.
TEST(InitializerTest, NonExistentFile) {
  // Access issues. IOErrors
  (core::Settings());
  EXPECT_THROW(Initializer({"./share/scram/input/nonexistent_file.xml"},
                           core::Settings()),
               IOError);
}

// Test if passing the same file twice causing an error.
TEST(InitializerTest, PassTheSameFileTwice) {
  std::string input_correct = "./share/scram/input/fta/correct_tree_input.xml";
  std::string the_same_path = "./share/.."  // Path obfuscation.
                              "/share/scram/input/fta/correct_tree_input.xml";
  EXPECT_THROW(Initializer({input_correct, the_same_path}, core::Settings()),
               ValidityError);
}

// Test if the schema catches errors.
// This is trusted to XML libraries and the correctness of the RELAX NG schema,
// so the test is very basic calls.
TEST(InitializerTest, FailSchemaValidation) {
  EXPECT_THROW(
      Initializer({"./share/scram/input/schema_fail.xml"}, core::Settings()),
      xml::ValidityError);
}

// Unsupported operations.
TEST(InitializerTest, UnsupportedFeature) {
  std::string dir = "./share/scram/input/fta/";
  const char* incorrect_inputs[] = {"../unsupported_feature.xml",
                                    "../unsupported_gate.xml",
                                    "../unsupported_expression.xml"};
  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, core::Settings()),
                 xml::ValidityError)
        << " Filename:  " << input;
  }
}

TEST(InitializerTest, EmptyAttributeElementText) {
  std::string dir = "./share/scram/input/fta/";
  const char* incorrect_inputs[] = {"../empty_element.xml",
                                    "../empty_attribute.xml"};
  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, core::Settings()),
                 xml::ValidityError)
        << " Filename:  " << input;
  }
}

TEST(InitializerTest, CorrectEtaInputs) {
  std::string dir = "./share/scram/input/eta/";
  const char* correct_inputs[] = {"simplest_correct.xml",
                                  "public_sequence.xml",
                                  "initiating_event.xml",
                                  "set_house_event.xml",
                                  "collect_formula.xml",
                                  "single_expression.xml",
                                  "if_then_else_instruction.xml",
                                  "block_instruction.xml",
                                  "rule_instruction.xml",
                                  "link_instruction.xml",
                                  "link_in_rule.xml",
                                  "test_initiating_event.xml",
                                  "test_functional_event.xml"};
  for (const auto& input : correct_inputs) {
    EXPECT_NO_THROW(Initializer({dir + input}, core::Settings()))
        << " Filename: " << input;
  }
}

TEST(InitializerTest, IncorrectEtaInputs) {
  std::string dir = "./share/scram/input/eta/";
  const char* incorrect_inputs[] = {
      "doubly_defined_initiating_event.xml",
      "doubly_defined_event_tree.xml",
      "doubly_defined_sequence.xml",
      "doubly_defined_functional_event.xml",
      "doubly_defined_branch.xml",
      "doubly_defined_path_state.xml",
      "doubly_defined_rule.xml",
      "undefined_event_tree.xml",
      "undefined_sequence.xml",
      "undefined_branch.xml",
      "undefined_functional_event.xml",
      "undefined_rule.xml",
      "undefined_house_in_set_house.xml",
      "private_branch.xml",
      "private_functional_event.xml",
      "cyclic_branches_fork.xml",
      "cyclic_branches_self.xml",
      "cyclic_branches_transitive.xml",
      "cyclic_rule_block.xml",
      "cyclic_rule_self.xml",
      "cyclic_rule_transitive.xml",
      "cyclic_link_self.xml",
      "cyclic_link_transitive.xml",
      "invalid_duplicate_event_in_forks.xml",
      "invalid_event_order_in_branch.xml",
      "invalid_event_order_in_link.xml",
      "invalid_event_order_in_initial_state.xml",
      "invalid_event_order_in_ref_branch.xml",
      "invalid_collect_formula.xml",
      "invalid_link_undefined_event_tree.xml",
      "invalid_link_instruction.xml",
      "invalid_link_in_branch.xml",
      "invalid_link_in_rule.xml",
      "undefined_arg_collect_formula.xml",
      "mixing_collect_instructions.xml",
      "mixing_collect_instructions_link.xml",
      "mixing_collect_instructions_fork.xml"};
  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, core::Settings()), ValidityError)
        << " Filename: " << input;
  }
}

TEST(InitializerTest, CorrectLabelsAndAttributes) {
  const char* input = "./share/scram/input/fta/labels_and_attributes.xml";
  EXPECT_NO_THROW(Initializer({input}, core::Settings()));
}

// Test correct inputs without probability information.
TEST(InitializerTest, CorrectFtaInputs) {
  std::string dir = "./share/scram/input/fta/";
  const char* correct_inputs[] = {
      "correct_tree_input.xml",
      "correct_formulas.xml",
      "constant_in_formulas.xml",
      "component_definition.xml",
      "mixed_definitions.xml",
      "mixed_references.xml",
      "mixed_roles.xml",
      "model_data_mixed_definitions.xml",
      "two_trees.xml",
      "two_top_events.xml",
      "two_top_through_formula.xml",
      "orphan_primary_event.xml",
      "very_long_mcs.xml",
      "unordered_structure.xml",
      "ccf_unordered_factors.xml",
      "missing_ccf_level_number.xml",
      "non_top_gate.xml",
      "unused_parameter.xml",
      "nested_formula.xml",
      "null_gate_with_label.xml",
      "case_sensitivity.xml",
      "weibull_lnorm_deviate_2p.xml",
      "weibull_lnorm_deviate_3p.xml"};

  for (const auto& input : correct_inputs) {
    EXPECT_NO_THROW(Initializer({dir + input}, core::Settings()))
        << " Filename: " << input;
  }
}

TEST(InitializerTest, CorrectInclude) {
  std::string dir = "./share/scram/input/";
  const char* correct_inputs[] = {"xinclude.xml", "xinclude_transitive.xml"};
  for (const auto& input : correct_inputs) {
    EXPECT_NO_THROW(Initializer({dir + input}, core::Settings()))
        << " Filename: " << input;
  }
}

TEST(InitializerTest, IncorrectInclude) {
  std::string dir = "./share/scram/input/";
  const char* correct_inputs[] = {"xinclude_no_file.xml", "xinclude_cycle.xml"};
  for (const auto& input : correct_inputs) {
    EXPECT_THROW(Initializer({dir + input}, core::Settings()),
                 xml::XIncludeError)
        << " Filename: " << input;
  }
}

// Test correct inputs with probability information.
TEST(InitializerTest, CorrectProbabilityInputs) {
  std::string dir = "./share/scram/input/fta/";
  const char* correct_inputs[] = {
      "missing_bool_constant.xml",  // House event is implicitly false.
      "correct_tree_input_with_probs.xml",
      "trailing_spaces.xml",
      "correct_expressions.xml",
      "flavored_types.xml"};

  core::Settings settings;
  settings.probability_analysis(true);

  for (const auto& input : correct_inputs) {
    EXPECT_NO_THROW(Initializer({dir + input}, settings))
        << " Filename: " << input;
  }
}

// Test incorrect fault tree inputs
TEST(InitializerTest, IncorrectFtaInputs) {
  std::string dir = "./share/scram/input/fta/";

  const char* incorrect_inputs[] = {
      "invalid_probability.xml",
      "private_at_model_scope.xml",
      "doubly_defined_gate.xml",
      "doubly_defined_house.xml",
      "doubly_defined_basic.xml",
      "doubly_defined_parameter.xml",
      "doubly_defined_ccf_group.xml",
      "doubly_defined_component.xml",
      "extra_ccf_level_beta_factor.xml",
      "missing_gate_definition.xml",
      "missing_ccf_factor.xml",
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
      "invalid_periodic_test_num_args.xml",
      "repeated_child.xml",
      "repeated_attribute.xml",
      "alpha_ccf_level_error.xml",
      "beta_ccf_level_error.xml",
      "mgl_ccf_level_error.xml",
      "phi_ccf_wrong_sum.xml",
      "ccf_negative_factor.xml",
      "ccf_more_factors_than_needed.xml",
      "ccf_wrong_distribution.xml",
      "repeated_ccf_members.xml"};

  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, core::Settings()), ValidityError)
        << " Filename:  " << input;
  }
}

TEST(InitializerTest, IncorrectXMLOverflow) {
  std::string input = "./share/scram/input/fta/int_overflow.xml";
  EXPECT_THROW(Initializer({input}, core::Settings()), xml::ValidityError);
}

// Test failures triggered only in with probability analysis.
TEST(InitializerTest, IncorrectProbabilityInputs) {
  std::string dir = "./share/scram/input/fta/";
  const char* incorrect_inputs[] = {"missing_expression.xml"};

  core::Settings settings;
  settings.probability_analysis(true);
  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, settings), ValidityError)
        << " Filename:  " << input;
  }
}

// Test the case when a top event is not orphan.
// The top event of one fault tree
// can be a child of a gate of another fault tree.
TEST(InitializerTest, NonOrphanTopEvent) {
  std::string dir = "./share/scram/input/fta/";
  EXPECT_NO_THROW(Initializer(
      {dir + "correct_tree_input.xml", dir + "second_fault_tree.xml"},
      core::Settings()));
}

TEST(InitializerTest, CorrectModelInputs) {
  std::string dir = "./share/scram/input/model/";
  const char* correct_inputs[] = {
      "extern_library.xml",
      "extern_function.xml",
      "extern_expression.xml",
      "valid_alignment.xml",
      "valid_sum_alignment.xml",
      "private_phases.xml",
      "substitution.xml",
      "substitution_optional_source.xml",
      "substitution_types.xml",
      "substitution_declarative_target_is_another_source.xml",
      "substitution_target_is_hypothesis.xml",
      "substitution_declarative_ccf.xml",
  };

  core::Settings settings;
  settings.approximation(core::Approximation::kRareEvent);
  for (const auto& input : correct_inputs) {
    EXPECT_NO_THROW(Initializer({dir + input}, settings, true)) << " Filename: "
                                                                << input;
  }
}

TEST(InitializerTest, IncorrectModelInputs) {
  std::string dir = "./share/scram/input/model/";

  const char* incorrect_inputs[] = {
      "duplicate_extern_libraries.xml",
      "duplicate_extern_functions.xml",
      "undefined_extern_library.xml",
      "undefined_symbol_extern_function.xml",
      "invalid_num_param_extern_function.xml",
      "undefined_extern_function.xml",
      "invalid_num_args_extern_expression.xml",
      "extern_library_invalid_path_format.xml",
      "duplicate_phases.xml",
      "invalid_phase_fraction.xml",
      "zero_phase_fraction.xml",
      "negative_phase_fraction.xml",
      "undefined_target_set_house_event.xml",
      "duplicate_alignment.xml",
      "excess_alignment.xml",
      "incomplete_alignment.xml",
      "duplicate_substitution.xml",
      "substitution_undefined_hypothesis_event.xml",
      "substitution_undefined_source_event.xml",
      "substitution_undefined_target_event.xml",
      "substitution_duplicate_source_event.xml",
      "substitution_duplicate_hypothesis_event.xml",
      "substitution_nested_formula.xml",
      "substitution_non_basic_event_formula.xml",
      "substitution_type_mismatch.xml",
      "substitution_no_effect.xml",
      "substitution_nondeclarative_complex.xml",
      "substitution_source_equal_target.xml",
      "substitution_target_is_another_source.xml",
      "substitution_target_is_another_hypothesis.xml",
      "substitution_source_is_another_hypothesis.xml",
      "substitution_source_false_target.xml",
      "substitution_declarative_noncoherent.xml",
      "substitution_nondeclarative_ccf_hypothesis.xml",
      "substitution_nondeclarative_ccf_source.xml",
      "substitution_nondeclarative_ccf_target.xml",
  };

  core::Settings settings;
  settings.approximation(core::Approximation::kRareEvent);
  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, settings, true), ValidityError)
        << " Filename:  " << input;
  }
}

TEST(InitializerTest, IncorrectModelEmptyInputs) {
  std::string dir = "./share/scram/input/model/";
  const char* incorrect_inputs[] = {"empty_extern_function.xml",
                                    "empty_alignment.xml"};

  for (const auto& input : incorrect_inputs) {
    EXPECT_THROW(Initializer({dir + input}, core::Settings(), true),
                 xml::ValidityError)
        << " Filename:  " << input;
  }
}

TEST(InitializerTest, ExternDLError) {
  std::string input = "./share/scram/input/model/extern_library_ioerror.xml";
  EXPECT_THROW(Initializer({input}, core::Settings(), true), DLError);
}

// Tests that external libraries are disabled by default.
TEST(InitializerTest, DefaultExternDisable) {
  std::string input = "./share/scram/input/model/extern_library.xml";
  EXPECT_NO_THROW(Initializer({input}, core::Settings(), true));
  EXPECT_THROW(Initializer({input}, core::Settings()), IllegalOperation);
}

// Non-declarative substitutions with approximations only.
TEST(InitializerTest, NonDeclarativeSubstitutionsWithAppproximations) {
  std::string input = "./share/scram/input/model/substitution_types.xml";
  EXPECT_THROW(Initializer({input}, core::Settings()), ValidityError);

  core::Settings settings;

  settings.approximation(core::Approximation::kRareEvent);
  EXPECT_NO_THROW(Initializer({input}, settings));
  settings.approximation(core::Approximation::kMcub);
  EXPECT_NO_THROW(Initializer({input}, settings));

  settings.prime_implicants(true);
  EXPECT_THROW(Initializer({input}, core::Settings()), ValidityError);
}

}  // namespace test
}  // namespace mef
}  // namespace scram
