#!usr/bin/env python

import os
from subprocess import call

from nose.tools import assert_equal, assert_not_equal

def test_fta_calls():
    """Tests all possible calls from the cli."""

    # Correct inputs
    fta_input = "./input/fta/correct_tree_input_with_probs.xml"
    fta_no_prob = "./input/fta/correct_tree_input.xml"

    # Test help
    cmd = ["scram", "-h"]
    yield assert_equal, 0, call(cmd)

    # Test version information
    cmd = ["scram", "--version"]
    yield assert_equal, 0, call(cmd)

    # Empty call
    cmd = ["scram"]
    yield assert_equal, 1, call(cmd)

    # Test the validation a fta tree file
    cmd = ["scram", "-v", fta_input]
    yield assert_equal, 0, call(cmd)

    # Test the incorrect limit order
    cmd = ["scram", fta_input, "-l", "-1"]
    yield assert_equal, 1, call(cmd)

    # Invalid argument type for an option
    cmd = ["scram", fta_input, "-l", "string_for_int"]
    yield assert_equal, 1, call(cmd)

    # Test the limit order no minimal cut sets.
    # This was an issue #17. This should not throw an error anymore.
    cmd = ["scram", fta_input, "-l", "1"]
    yield assert_equal, 0, call(cmd)

    # Test the incorrect cut-off probability
    cmd = ["scram", fta_input, "-c", "-1"]
    yield assert_equal, 1, call(cmd)
    cmd = ["scram", fta_input, "-c", "10"]
    yield assert_equal, 1, call(cmd)

    # Test the incorrect number for probability series
    cmd = ["scram", fta_input, "-s", "-1"]
    yield assert_equal, 1, call(cmd)

    # Test incorrect type of analysis
    cmd = ["scram", fta_input, "-a", "fta-nonexistent-analysis"]
    yield assert_equal, 1, call(cmd)

    # Test graph only
    cmd = ["scram", "-g", fta_input]
    yield assert_equal, 0, call(cmd)
    graph_file = "./input/fta/correct_tree_input_with_probs_TwoTrains.dot"
    # Changing permission
    cmd = ["chmod", "a-w", graph_file]
    call(cmd)
    cmd = ["scram", "-g", fta_input]
    yield assert_not_equal, 0, call(cmd)
    if os.path.isfile(graph_file):
        os.remove(graph_file)

    # Test calculation calls
    cmd = ["scram", fta_no_prob]
    yield assert_equal, 0, call(cmd)
    out_temp = "./output_temp.txt"
    cmd.append("-o")
    cmd.append(out_temp)
    yield assert_equal, 0, call(cmd)  # Report into an outupt file
    if os.path.isfile(out_temp):
        os.remove(out_temp)

    # Test MC
    cmd = ["scram", fta_input, "-a", "fta-mc"]
    yield assert_equal, 0, call(cmd)
    cmd = ["scram", fta_no_prob, "-a", "fta-mc"]
    yield assert_equal, 0, call(cmd)
