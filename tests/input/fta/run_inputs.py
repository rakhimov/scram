#!/usr/bin/env python
"""run_inputs.py

Runs SCRAM with test inputs to examine the output visually.
"""
from __future__ import print_function

import subprocess
import sys

def main():
    # Correct corner case inputs without probability information
    pass_inputs = [
            "correct_tree_input.xml",
            "mixed_definitions.xml",
            "model_data_mixed_definitions.xml",
            "two_trees.xml",
            "labels_and_attributes.xml",
            "orphan_primary_event.xml",
            "very_long_mcs.xml",
            "unordered_structure.xml",
            "non_top_gate.xml",
            "unused_parameter.xml",
            ]

    # Correct corner cases with probability information
    pass_probs = [
            "correct_tree_input_with_probs.xml",
            "trailing_spaces.xml",
            "correct_expressions.xml",
            "flavored_types.xml",
            ]

    # Incorrect input files without probability calculations
    bad_inputs = [
            "../xml_formatting_error.xml",
            "../schema_fail.xml",
            "../unsupported_feature.xml",
            "../unsupported_gate.xml",
            "../unsupported_expression.xml",
            "nonexistent_file.xml",
            "doubly_defined_gate.xml",
            "doubly_defined_house.xml",
            "doubly_defined_basic.xml",
            "doubly_defined_parameter.xml",
            "doubly_defined_ccf_group.xml",
            "extra_ccf_level_beta_factor.xml",
            "missing_gate_definition.xml",
            "missing_ccf_level_number.xml",
            "missing_ccf_members.xml",
            "undefined_event.xml",
            "undefined_basic_event.xml",
            "undefined_house_event.xml",
            "undefined_gate.xml",
            "undefined_parameter.xml",
            "name_clash_two_trees.xml",
            "def_clash_basic_gate.xml",
            "def_clash_house_gate.xml",
            "def_clash_gate_primary.xml",
            "def_clash_basic_house.xml",
            "def_clash_house_basic.xml",
            "atleast_gate.xml",
            "two_top_events.xml",
            "cyclic_tree.xml",
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
            ]

    # Incorrect input files with probability calculations
    bad_probs = [
            "invalid_probability.xml",
            "missing_expression.xml",
            "missing_bool_constant.xml",
            "ccf_wrong_distribution.xml"
            ]

    # Visual delimeters
    block_delim = "=" * 80
    file_delim = "-" * 80

    # Run correct inputs
    print("\nRUNNING CORRECT INPUTS WITHOUT PROBABILITY CALCULATION")
    print(block_delim)
    for i in pass_inputs:
        print("\nRUNNING : " + i)
        print(file_delim)
        args = ["scram", i]
        subprocess.call(args)
        print(file_delim)
    print(block_delim)

    # Run incorrect inputs
    print("\nRUNNING INCORRECT INPUTS WITHOUT PROBABILITY CALCULATION")
    print(block_delim)
    for i in bad_inputs:
        print("\nRUNNING : " + i)
        print(file_delim)
        args = ["scram", i]
        try:
            subprocess.check_call(args)
        except subprocess.CalledProcessError:
            print(sys.exc_info()[0])
        print(file_delim)
    print(block_delim)

    # Run correct inputs with probabilities
    print("\nRUNNING CORRECT PROBABILITY INPUTS")
    print(block_delim)
    for i in pass_probs:
        print("\nRUNNING : " + i)
        print(file_delim)
        args = ["scram", i, "--probability", "1"]
        subprocess.call(args)
        print(file_delim)
    print(block_delim)

    # Run incorrect inputs with probability calculations
    print("\nRUNNING INCORRECT PROBABILITY INPUTS")
    print(block_delim)
    for i in bad_probs:
        print("\nRUNNING : " + i)
        print(file_delim)
        args = ["scram", i, "--probability", "1"]
        try:
            subprocess.check_call(args)
        except subprocess.CalledProcessError:
            print(sys.exc_info()[0])
        print(file_delim)
    print(block_delim)

if __name__ == "__main__":
    main()
