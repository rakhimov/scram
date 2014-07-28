#!/usr/bin/env python

import sys
import subprocess

def main():
    # Correct corner case inputs
    pass_inputs = [
            "correct_tree_input.scramf",
            "different_order.scramf",
            "doubly_defined_basic.scramf",
            "inline_comments.scramf",
            "transfer_correct_top.scramf",
            "transfer_correct_sub.scramf",
            ]

    pass_probs = [
            "correct_prob_input.scramp",
            "correct_lambda_prob.scramp",
            ]

    # Wrong input files in the current directory
    bad_inputs = [
            "basic_top_event.scramf",
            "conditional_wrong_type_inter.scramf",
            "conditional_wrong_type_top.scramf",
            "doubly_defined_intermediate.scramf",
            "doubly_defined_primary_type.scramf",
            "doubly_defined_top.scramf",
            "extra_parameter.scramf",
            "leaf_intermidiate_event.scramf",
            "missing_closing_brace.scramf",
            "missing_closing_brace_at_end.scramf",
            "missing_id.scramf",
            "missing_nodes.scramf",
            "missing_opening_brace.scramf",
            "missing_opening_brace_at_start.scramf",
            "missing_parameter.scramf",
            "missing_parent.scramf",
            "missing_type.scramf",
            "name_clash_inter.scramf",
            "name_clash_primary.scramf",
            "name_clash_top.scramf",
            "non_existent_parent_inter.scramf",
            "non_existent_parent_primary.scramf",
            "nonexistent_file.scramf",
            "one_arg_err.scramf",
            "second_closing_brace.scramf",
            "too_many_args.scramf",
            "top_event_with_no_child.scramf",
            "transfer_circular_self_top.scramf",
            "transfer_circular_top.scramf",
            "transfer_extra_second_node_top.scramf",
            "transfer_head_extra_nodes.scramf",
            "transfer_extra_transferout.scramf",
            "transfer_illegal_reference_top.scramf",
            "transfer_name_mismatch_top.scramf",
            "transfer_no_file.scramf",
            "transfer_primary_second_node_top.scramf",
            "transfer_second_transferout_top.scramf",
            "transfer_wrong_parent.scramf",
            "transfer_wrong_root_top.scramf",
            "transfer_wrong_second_node_top.scramf",
            "transfer_wrong_type_top.scramf",
            "unrecognized_parameter.scramf",
            "unrecognized_type.scramf",
            "transfer_circular_self_bottom.scramf",
            "transfer_circular_middle.scramf",
            "transfer_circular_bottom.scramf",
            "transfer_sub_wrong_parent.scramf",
            "transfer_name_mismatch_sub.scramf",
            "transfer_wrong_type_sub.scramf",
            "transfer_wrong_second_node_sub.scramf",
            "transfer_primary_second_node_sub.scramf",
            "transfer_second_transferout_sub.scramf",
            "transfer_extra_second_node_sub.scramf",
            "transfer_illegal_reference_sub.scramf",
            "vote_no_number.scramf",
            "vote_not_enough_children.scramf",
            "vote_string.scramf",
            ]

    # Wrong probability files in the current directory
    bad_probs = [
            "doubly_defined_block.scramp",
            "doubly_defined_prob.scramp",
            "doubly_defined_time.scramp",
            "huge_prob.scramp",
            "missing_basic_event.scramp",
            "missing_closing_brace.scramp",
            "missing_closing_brace_at_end.scramp",
            "missing_opening_brace.scramp",
            "missing_opening_brace_at_start.scramp",
            "negative_prob.scramp",
            "negative_time.scramp",
            "no_time_given.scramp",
            "nonexistent_file.scramp",
            "one_arg_err.scramp",
            "second_closing_brace.scramp",
            "string_prob.scramp",
            "string_time.scramp",
            "too_many_args.scramp",
            "unrecognized_block.scramp",
            ]

    input_correct = "correct_tree_input.scramf"
    prob_correct = "correct_prob_input.scramp"
    lambda_correct = "correct_lambda_prob.scramp"

    # Run correct inputs
    print("\nRunning Correct Inputs\n\n")
    for ic in pass_inputs:
        print("\nRunning : " + ic + "\n")
        args = ["scram", ic]
        subprocess.call(args)

    # Run correct probabilities
    print("\nRunning Correct Probability Inputs\n\n")
    for p in pass_probs:
        print("\nRunning : " + p + "\n")
        args = ["scram", input_correct, p]
        subprocess.call(args)

    # Run incorrect inputs
    print("\nRunning Incorrect Inputs\n\n")
    for i in bad_inputs:
        print("\nRunning : " + i + "\n")
        msg = ""
        args = ["scram", i]
        try:
            subprocess.call(args)
        except:
            msg = sys.exc_info()[0]
        print(msg)

    # Run incorrect probabilities
    print("\nRunning Incorrect Probabilities\n\n")
    for p in bad_probs:
        print("\nRunning : " + p + "\n")
        msg = ""
        args = ["scram", input_correct, p]
        try:
            subprocess.call(args)
        except:
            msg = sys.exc_info()[0]
        print(msg)


if __name__ == "__main__":
    main()
