#!usr/bin/env python

import os
from subprocess import call

from nose.tools import assert_equal

def test_fta_calls():
    """Tests all possible calls from the cli."""

    # Correct inputs
    fta_input = "./input/fta/correct_tree_input.scramf"
    fta_prob = "./input/fta/correct_prob_input.scramp"

    # Test help
    cmd = ["scram", "-h"]
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

    # Test the incorrect number for probability series
    cmd = ["scram", fta_input, fta_prob, "-s", "-1"]
    yield assert_equal, 1, call(cmd)

    # Test incorrect type of analysis
    cmd = ["scram", fta_input, fta_prob, "-a", "fta-nonexistent-analysis"]
    yield assert_equal, 1, call(cmd)

    # Test graph only
    cmd = ["scram", "-g", fta_input]
    yield assert_equal, 0, call(cmd)
    graph_file = "./input/fta/correct_tree_input.dot"
    if os.path.isfile(graph_file):
        os.remove(graph_file)

    # Test validation of a probability file
    cmd = ["scram", "-v", fta_input, fta_prob]
    yield assert_equal, 0, call(cmd)

    # Test calculation calls
    cmd = ["scram", fta_input]
    yield assert_equal, 0, call(cmd)
    cmd.append(fta_prob)
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
    cmd = ["scram", fta_input, fta_prob, "-a", "fta-mc"]
    yield assert_equal, 0, call(cmd)
