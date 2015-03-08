#!/usr/bin/env python

import sys
import subprocess

def main():
    # Correct corner case inputs
    pass_inputs = [
            "correct_tree_input.xml",
            "two_trees.xml",
            ]

    pass_probs = [
            "correct_tree_input_with_probs.xml",
            "mixed_definitions.xml",
            "model_data_mixed_definitions.xml",
            "trailing_spaces.xml",
            "labels_and_attributes.xml",
            "orphan_primary_event.xml",
            "correct_expressions.xml",
            "flavored_types.xml",
            "very_long_mcs.xml",
            ]

    # Wrong input files in the current directory
    bad_inputs = [
            "../xml_formatting_error.xml",
            "../schema_fail.xml",
            "../unsupported_feature.xml",
            "unsupported_gate.xml",
            "unsupported_expression.xml",
            "nonexistent_file.xml",
            "doubly_defined_gate.xml",
            "doubly_defined_house.xml",
            "doubly_defined_basic.xml",
            "doubly_defined_parameter.xml",
            "doubly_defined_ccf_group.xml",
            "extra_ccf_level_beta_factor.xml",
            "missing_event_definition.xml",
            "missing_basic_event_definition.xml",
            "missing_house_event_definition.xml",
            "missing_expression.xml",
            "missing_bool_constant.xml",
            "missing_parameter.xml",
            "missing_gate_definition.xml",
            "missing_ccf_level_number.xml",
            "missing_ccf_members.xml",
            "name_clash_basic_gate.xml",
            "name_clash_house_gate.xml",
            "name_clash_gate_primary.xml",
            "name_clash_basic_house.xml",
            "name_clash_house_basic.xml",
            "name_clash_two_trees.xml",
            "def_clash_basic_gate.xml",
            "def_clash_house_gate.xml",
            "def_clash_gate_primary.xml",
            "def_clash_basic_house.xml",
            "def_clash_house_basic.xml",
            "def_name_house_basic.xml",
            "def_name_basic_house.xml",
            "atleast_gate.xml",
            "unordered_structure.xml",
            "dangling_gate.xml",
            "non_top_gate.xml",
            "cyclic_tree.xml",
            "cyclic_parameter.xml",
            "invalid_probability.xml",
            "invalid_expression.xml",
            "repeated_child.xml",
            "alpha_ccf_level_error.xml",
            "beta_ccf_level_error.xml",
            "mgl_ccf_level_error.xml",
            "phi_ccf_wrong_sum.xml",
            "ccf_negative_factor.xml",
            "ccf_wrong_distribution.xml",
            "ccf_more_factors_than_needed.xml",
            ]

    # Run correct inputs
    print("\n\nRUNNING CORRECT INPUTS\n")
    for ic in pass_inputs:
        print("\nRUNNING : " + ic + "\n")
        args = ["scram", ic]
        subprocess.call(args)

    # Run correct probabilities
    print("\n\nRUNNING CORRECT PROBABILITY INPUTS\n")
    for p in pass_probs:
        print("\nRUNNING : " + p + "\n")
        args = ["scram", p, "--probability", "1"]
        subprocess.call(args)

    # Run incorrect inputs
    print("\n\nRUNNING INCORRECT INPUTS\n")
    for i in bad_inputs:
        print("\nRUNNING : " + i + "\n")
        msg = ""
        args = ["scram", i, "--probability", "1"]
        try:
            subprocess.call(args)
        except:
            msg = sys.exc_info()[0]
        print(msg)

if __name__ == "__main__":
    main()
