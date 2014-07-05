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

    # Test the validation
    cmd = ["scram", "-v", fta_input]
    yield assert_equal, 0, call(cmd)

    # Test graph only
    cmd = ["scram", "-g", fta_input]
    yield assert_equal, 0, call(cmd)
    graph_file = "./input/fta/correct_tree_input.dot"
    if os.path.isfile(graph_file):
        os.remove(graph_file)

    cmd = ["scram", "-v", fta_input, fta_prob]
    yield assert_equal, 0, call(cmd)

    # Test calculation calls
    cmd = ["scram", fta_input]
    yield assert_equal, 0, call(cmd)
    assert_equal(0, call(cmd));
    yield assert_equal, 0, call(cmd)

    # Test MC
    cmd = ["scram", fta_input, "-a", "fta-mc"]
    yield assert_equal, 0, call(cmd)
    cmd = ["scram", fta_input, fta_prob, "-a", "fta-mc"]
    yield assert_equal, 0, call(cmd)
