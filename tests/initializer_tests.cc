/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

#include <catch.hpp>

#include "error.h"
#include "settings.h"

namespace scram::mef::test {

// Test if the XML is well formed.
TEST_CASE("InitializerTest.XMLFormatting", "[mef::initializer]") {
  CHECK_THROWS_AS(
      Initializer({"tests/input/xml_formatting_error.xml"}, core::Settings()),
      xml::ParseError);
}

// XML custom namespace errors.
TEST_CASE("InitializerTest.XMLNameSpace", "[mef::initializer]") {
  CHECK_THROWS_AS(
      Initializer({"tests/input/undefined_xmlns.xml"}, core::Settings()),
      xml::ParseError);
  CHECK_THROWS_AS(
      Initializer({"tests/input/custom_xmlns.xml"}, core::Settings()),
      xml::ValidityError);
}

// Test the response for non-existent file.
TEST_CASE("InitializerTest.NonExistentFile", "[mef::initializer]") {
  // Access issues. IOErrors
  CHECK_THROWS_AS(
      Initializer({"tests/input/nonexistent_file.xml"}, core::Settings()),
      IOError);
}

// Test if passing the same file twice causing an error.
TEST_CASE("InitializerTest.PassTheSameFileTwice", "[mef::initializer]") {
  std::string input_correct = "tests/input/fta/correct_tree_input.xml";
  std::string the_same_path =  // Path obfuscation.
      "tests/../tests/input/fta/correct_tree_input.xml";
  CHECK_THROWS_AS(Initializer({input_correct, the_same_path}, core::Settings()),
                  IOError);
}

// Test if the schema catches errors.
// This is trusted to XML libraries and the correctness of the RELAX NG schema,
// so the test is very basic calls.
TEST_CASE("InitializerTest.FailSchemaValidation", "[mef::initializer]") {
  std::string dir = "tests/input/";
  const char* incorrect_inputs[] = {
      "schema_fail.xml",
      "fta/nested_formula.xml",
      "fta/nested_not_not.xml",
      "fta/nested_not_constant.xml",
  };
  for (const auto& input : incorrect_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings()),
                    xml::ValidityError);
  }
}

// Unsupported operations.
TEST_CASE("InitializerTest.UnsupportedFeature", "[mef::initializer]") {
  std::string dir = "tests/input/";
  const char* incorrect_inputs[] = {"unsupported_feature.xml",
                                    "unsupported_gate.xml",
                                    "unsupported_expression.xml"};
  for (const auto& input : incorrect_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings()),
                    xml::ValidityError);
  }
}

TEST_CASE("InitializerTest.EmptyAttributeElementText", "[mef::initializer]") {
  std::string dir = "tests/input/";
  const char* incorrect_inputs[] = {"empty_element.xml", "empty_attribute.xml"};
  for (const auto& input : incorrect_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings()),
                    xml::ValidityError);
  }
}

TEST_CASE("InitializerTest.CorrectEtaInputs", "[mef::initializer]") {
  std::string dir = "tests/input/eta/";
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
    CAPTURE(input);
    CHECK_NOTHROW(Initializer({dir + input}, core::Settings()));
  }
}

TEST_CASE("InitializerTest.IncorrectEtaInputs", "[mef::initializer]") {
  std::string dir = "tests/input/eta/";
  const char* incorrect_inputs[] = {"doubly_defined_initiating_event.xml",
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
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings()),
                    ValidityError);
  }
}

TEST_CASE("InitializerTest.CorrectLabelsAndAttributes", "[mef::initializer]") {
  const char* input = "tests/input/fta/labels_and_attributes.xml";
  CAPTURE(input);
  CHECK_NOTHROW(Initializer({input}, core::Settings()));
}

// Test correct inputs without probability information.
TEST_CASE("InitializerTest.CorrectFtaInputs", "[mef::initializer]") {
  std::string dir = "tests/input/fta/";
  const char* correct_inputs[] = {"correct_tree_input.xml",
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
                                  "null_gate_with_label.xml",
                                  "case_sensitivity.xml",
                                  "weibull_lnorm_deviate_2p.xml",
                                  "weibull_lnorm_deviate_3p.xml"};

  for (const auto& input : correct_inputs) {
    CAPTURE(input);
    CHECK_NOTHROW(Initializer({dir + input}, core::Settings()));
  }
}

TEST_CASE("InitializerTest.CorrectInclude", "[mef::initializer]") {
  std::string dir = "tests/input/";
  const char* correct_inputs[] = {"xinclude.xml", "xinclude_transitive.xml"};
  for (const auto& input : correct_inputs) {
    CAPTURE(input);
    CHECK_NOTHROW(Initializer({dir + input}, core::Settings()));
  }
}

TEST_CASE("InitializerTest.IncorrectInclude", "[mef::initializer]") {
  std::string dir = "tests/input/";
  const char* correct_inputs[] = {"xinclude_no_file.xml", "xinclude_cycle.xml"};
  for (const auto& input : correct_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings()),
                    xml::XIncludeError);
  }
}

// Test correct inputs with probability information.
TEST_CASE("InitializerTest.CorrectProbabilityInputs", "[mef::initializer]") {
  std::string dir = "tests/input/fta/";
  const char* correct_inputs[] = {
      "missing_bool_constant.xml",  // House event is implicitly false.
      "correct_tree_input_with_probs.xml", "trailing_spaces.xml",
      "correct_expressions.xml", "flavored_types.xml"};

  core::Settings settings;
  settings.probability_analysis(true);

  for (const auto& input : correct_inputs) {
    CAPTURE(input);
    CHECK_NOTHROW(Initializer({dir + input}, settings));
  }
}

// Test incorrect fault tree inputs
TEST_CASE("InitializerTest.IncorrectFtaInputs", "[mef::initializer]") {
  std::string dir = "tests/input/fta/";
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
      "invalid_min_max_cardinality.xml",
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
      "repeated_ccf_members.xml",
      "duplicate_via_not.xml",
  };

  for (const auto& input : incorrect_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings()),
                    ValidityError);
  }
}

TEST_CASE("InitializerTest.IncorrectXMLOverflow", "[mef::initializer]") {
  std::string input = "tests/input/fta/int_overflow.xml";
  CHECK_THROWS_AS(Initializer({input}, core::Settings()), xml::ValidityError);
}

// Test failures triggered only in with probability analysis.
TEST_CASE("InitializerTest.IncorrectProbabilityInputs", "[mef::initializer]") {
  std::string dir = "tests/input/fta/";
  const char* incorrect_inputs[] = {"missing_expression.xml"};

  core::Settings settings;
  settings.probability_analysis(true);
  for (const auto& input : incorrect_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, settings), ValidityError);
  }
}

// Test the case when a top event is not orphan.
// The top event of one fault tree
// can be a child of a gate of another fault tree.
TEST_CASE("InitializerTest.NonOrphanTopEvent", "[mef::initializer]") {
  std::string dir = "tests/input/fta/";
  CHECK_NOTHROW(Initializer(
      {dir + "correct_tree_input.xml", dir + "second_fault_tree.xml"},
      core::Settings()));
}

TEST_CASE("InitializerTest.CorrectModelInputs", "[mef::initializer]") {
  std::string dir = "tests/input/model/";
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
    CAPTURE(input);
    CHECK_NOTHROW(Initializer({dir + input}, settings, true));
  }
}

TEST_CASE("InitializerTest.IncorrectModelInputs", "[mef::initializer]") {
  std::string dir = "tests/input/model/";
  const char* incorrect_inputs[] = {
      "duplicate_extern_libraries.xml",
      "duplicate_extern_functions.xml",
      "undefined_extern_library.xml",
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
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, settings, true), ValidityError);
  }
}

TEST_CASE("InitializerTest.IncorrectModelEmptyInputs", "[mef::initializer]") {
  std::string dir = "tests/input/model/";
  const char* incorrect_inputs[] = {"empty_extern_function.xml",
                                    "empty_alignment.xml"};

  for (const auto& input : incorrect_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(Initializer({dir + input}, core::Settings(), true),
                    xml::ValidityError);
  }
}

TEST_CASE("InitializerTest.ExternDLError", "[mef::initializer]") {
  CHECK_THROWS_AS(Initializer({"tests/input/model/extern_library_ioerror.xml"},
                              core::Settings(), true),
                  DLError);
  CHECK_THROWS_AS(
      Initializer({"tests/input/model/undefined_symbol_extern_function.xml"},
                  core::Settings(), true),
      DLError);
}

// Tests that external libraries are disabled by default.
TEST_CASE("InitializerTest.DefaultExternDisable", "[mef::initializer]") {
  std::string input = "tests/input/model/extern_library.xml";
  CHECK_NOTHROW(Initializer({input}, core::Settings(), true));
  CHECK_THROWS_AS(Initializer({input}, core::Settings()), IllegalOperation);
}

// Non-declarative substitutions with approximations only.
TEST_CASE("InitializerTest.NonDeclarativeSubstitutionsWithAppproximations",
          "[mef::initializer]") {
  std::string input = "tests/input/model/substitution_types.xml";
  CHECK_THROWS_AS(Initializer({input}, core::Settings()), ValidityError);

  core::Settings settings;

  settings.approximation(core::Approximation::kRareEvent);
  CHECK_NOTHROW(Initializer({input}, settings));
  settings.approximation(core::Approximation::kMcub);
  CHECK_NOTHROW(Initializer({input}, settings));

  settings.prime_implicants(true);
  CHECK_THROWS_AS(Initializer({input}, core::Settings()), ValidityError);
}

}  // namespace scram::mef::test
