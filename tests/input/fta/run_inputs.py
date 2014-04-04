#! /usr/env/python

import sys
import subprocess

def main():
    # correct corner case inputs
    pass_inputs = ["correct_tree_input.scramf",
                   "doubly_defined_basic.scramf",
                   "different_order.scramf",
                   "inline_comments.scramf"]

    # wrong input files in the current directory
    bad_inputs = ["basic_top_event.scramf",
                  "doubly_defined_intermediate.scramf",
                  "doubly_defined_top.scramf",
                  "extra_parameter.scramf",
                  "leaf_intermidiate_event.scramf",
                  "missing_closing_brace.scramf",
                  "missing_id.scramf",
                  "missing_nodes.scramf",
                  "missing_opening_brace.scramf",
                  "missing_parameter.scramf",
                  "missing_parent.scramf",
                  "missing_type.scramf",
                  "non_existent_parent.scramf",
                  "unrecognized_parameter.scramf",
                  "unrecognized_type.scramf",
                  "transfer_circular_self_top.scramf",
                  "transfer_circular_top.scramf",
                  "transfer_head_extra_nodes.scramf",
                  "transfer_no_file.scramf",
                  "transfer_wrong_parent.scramf"]

    # wrong probability files in the current directory
    bad_probs = ["doubly_defined_prob.scramp",
                 "extra_basic_event.scramp",
                 "huge_prob.scramp",
                 "missing_basic_event.scramp",
                 "string_prob.scramp",
                 "negative_prob.scramp"]

    input_correct = "correct_tree_input.scramf"
    prob_correct = "correct_prob_input.scramp"

    # Run correct inputs
    print("\nRunning Correct Inputs\n\n")
    for ic in pass_inputs:
        print("Running : " + ic + "\n")
        args = ["scram", ic, prob_correct]
        subprocess.call(args)

    # Run incorrect inputs
    print("\nRunning Incorrect Inputs\n\n")
    for i in bad_inputs:
        print("Running : " + i + "\n")
        msg = ""
        args = ["scram", i, prob_correct]
        try:
            subprocess.call(args)
        except:
            msg = sys.exc_info()[0]
        print(msg)

    # Run incorrect probabilities
    print("\nRunning Incorrect Probabilities\n\n")
    for p in bad_probs:
        print("Running : " + p + "\n")
        msg = ""
        args = ["scram", input_correct, p]
        try:
            subprocess.call(args)
        except:
            msg = sys.exc_info()[0]
        print(msg)


if __name__ == "__main__":
    main()
