#!/usr/bin/env python
"""test_scram_call.py

A set of tests to invoke command-line SCRAM with correct and
incorrect arguments.
"""

import os
from subprocess import call

from nose.tools import assert_true, assert_equal, assert_not_equal

def test_empty_call():
    """Tests a command-line call without any arguments."""
    cmd = ["scram"]
    yield assert_equal, 1, call(cmd)

def test_info_calls():
    """Tests general information calls about SCRAM."""
    # Test help
    cmd = ["scram", "--help"]
    yield assert_equal, 0, call(cmd)

    # Test version information
    cmd = ["scram", "--version"]
    yield assert_equal, 0, call(cmd)

def test_fta_no_prob():
    """Tests calls for fault tree analysis without probability information."""
    fta_no_prob = "./input/fta/correct_tree_input.xml"
    # Test calculation calls
    cmd = ["scram", fta_no_prob]
    yield assert_equal, 0, call(cmd)
    out_temp = "./output_temp.xml"
    cmd.append("-o")
    cmd.append(out_temp)
    yield assert_equal, 0, call(cmd)  # report into an output file
    if os.path.isfile(out_temp):
        os.remove(out_temp)

def test_fta_calls():
    """Tests calls for full fault tree analysis."""
    fta_input = "./input/fta/correct_tree_input_with_probs.xml"

    # Test the validation a fta tree file
    cmd = ["scram", "--validate", fta_input]
    yield assert_equal, 0, call(cmd)

    # Test the incorrect limit order
    cmd = ["scram", fta_input, "-l", "-1"]
    yield assert_not_equal, 0, call(cmd)

    # Invalid argument type for an option
    cmd = ["scram", fta_input, "-l", "string_for_int"]
    yield assert_equal, 1, call(cmd)

    # Test the limit order no minimal cut sets.
    # This was an issue #17. This should not throw an error anymore.
    cmd = ["scram", fta_input, "-l", "1"]
    yield assert_equal, 0, call(cmd)

    # Test the incorrect cut-off probability
    cmd = ["scram", fta_input, "-c", "-1"]
    yield assert_not_equal, 0, call(cmd)
    cmd = ["scram", fta_input, "-c", "10"]
    yield assert_not_equal, 0, call(cmd)

    # Test the incorrect number for probability series
    cmd = ["scram", fta_input, "-s", "-1"]
    yield assert_not_equal, 0, call(cmd)

    # Test the application of the rare event and MCUB at the same time
    cmd = ["scram", fta_input, "--rare-event", "--mcub"]
    yield assert_not_equal, 0, call(cmd)

    # Test the rare event approximation
    cmd = ["scram", fta_input, "--rare-event"]
    yield assert_equal, 0, call(cmd)

    # Test the MCUB approximation
    cmd = ["scram", fta_input, "--mcub"]
    yield assert_equal, 0, call(cmd)

def test_config_file():
    """Tests calls with configuration files."""
    # Test with a configuration file
    config_file = "./input/fta/full_configuration.xml"
    out_temp = "./output_temp.xml"
    cmd = ["scram", "--config-file", config_file, "-o", out_temp]
    yield assert_equal, 0, call(cmd)
    if os.path.isfile(out_temp):
        os.remove(out_temp)

    # Test the clash of files from configuration and command-line
    config_file = "./input/fta/full_configuration.xml"
    cmd = ["scram", "--config-file", config_file,
            "input/fta/correct_tree_input_with_probs.xml"]
    yield assert_not_equal, 0, call(cmd)

def test_logging():
    """Tests invokation with logging."""
    fta_input = "./input/fta/correct_tree_input_with_probs.xml"
    cmd = ["scram", fta_input, "--verbosity", "-1"]
    yield assert_equal, 1, call(cmd)
    cmd = ["scram", fta_input, "--verbosity", "8"]
    yield assert_equal, 1, call(cmd)
    cmd = ["scram", fta_input, "--verbosity", "2"]
    yield assert_equal, 0, call(cmd)

def test_graph_call():
    """Tests the calls for graphing instructions for a fault tree."""
    fta_input = "./input/fta/correct_tree_input_with_probs.xml"
    # Test graph only
    cmd = ["scram", fta_input, "--graph"]
    yield assert_equal, 0, call(cmd)
    graph_file = "./input/fta/TwoTrains.dot"
    cmd = ["scram", fta_input, "--graph", "-o", graph_file]
    yield assert_equal, 0, call(cmd)
    # Test if output is created
    yield assert_true, os.path.isfile(graph_file)
    # Changing permission
    cmd = ["chmod", "a-w", graph_file]
    call(cmd)
    cmd = ["scram", fta_input, "--graph", "-o", graph_file]
    yield assert_not_equal, 0, call(cmd)
    if os.path.isfile(graph_file):
        os.remove(graph_file)
