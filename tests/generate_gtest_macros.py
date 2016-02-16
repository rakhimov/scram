#!/usr/bin/env python

"""Generates a listing of ADD_TEST CMake macros from GoogleTest.

The macros are generated only for non-disabled tests
in a google-test-based executable.
"""

import os
import subprocess
import sys

import argparse as ap


def parse_tests(test_lines):
    """Parses lines to detect test names.

    Args:
        test_lines: a list of the output of a google test exectuable
            using the --gtest_list_tests flag. If the output is in a file,
            test_lines are the result of file.readlines().

    Returns:
        A list of google test names.
    """
    tests = []
    current_test = None
    for test_line in test_lines:
        line = test_line.decode().strip()
        if line[-1] == ".":
            current_test = line
        else:
            assert current_test is not None
            if str(line).lower().find("disabled") == -1:
                tests.append(current_test + line)
    return tests


def write_macros(tests, executable, output):
    """Writes a list of test names as ADD_TEST cmake macros to an output file.

    Args:
        tests: A list of all test names to be added as ADD_TEST macros
            to the output file.
        exectuable: The name of the test executable.
        output: The output file to write to.
    """
    lines = []
    for test in tests:
        lines.append("ADD_TEST(" + test + " " +
                     executable + " " + "--gtest_filter=" + test + ")")
    with open(output, "a") as out_file:
        for line in lines:
            out_file.write(line + "\n")


def main():
    """Writes a list of macros to a given output file."""
    parser = ap.ArgumentParser(
        description="A simple script to add CTest ADD_TEST macros to a "
                    "file for every test in a google-test executable.")
    parser.add_argument("--executable",
                        help="the path to the test executable to call",
                        required=True)
    parser.add_argument("--output",
                        help="the file to write the ADD_TEST macros to "
                             "(nominally CTestTestfile.cmake)",
                        required=True)
    args = parser.parse_args()

    assert os.path.exists(args.executable)
    assert os.path.exists(args.output)

    rtn = subprocess.Popen([args.executable, "--gtest_list_tests"],
                           stdout=subprocess.PIPE, shell=(os.name == "nt"))

    tests = parse_tests(rtn.stdout.readlines())
    rtn.stdout.close()

    write_macros(tests, args.executable, args.output)
    return 0

if __name__ == "__main__":
    sys.exit(main())
