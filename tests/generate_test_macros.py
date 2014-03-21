"""generate_test_macros.py

A simple module and default main execution to generate a listing of 
ADD_TEST CMake macros for all non-disabled tests in a 
google-test-based executable.

The default main function writes a list of macros to the given output 
file. 
"""
from __future__ import print_function

import os
import subprocess
import sys

try:
    import argparse as ap
except ImportError:
    import pyne._argparse as ap

def parse_tests(test_lines):
    """Return a list of google test names.

    Arguments: 

    test_lines -- a list of the output of a google test exectuable 
    using the --gtest_list_tests flag. If the output is in a file, 
    test_lines are the result of file.readlines().
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

def write_macros_to_output(tests, executable, output=None):
    """writes a list of test names as ADD_TEST cmake macros to an 
    output file
    
    Arguments
    tests -- a list of all test names to be added as ADD_TEST macros 
    to the output file
    exectuable -- the name of the test executable
    output -- the output file to write to, if output is not 
    specified, the list of ADD_TEST macros will be written to stdout
    """
    lines = []
    for test in tests:
        lines.append("ADD_TEST(" + test + " " + \
                         executable + " " + "--gtest_filter=" + test + ")")
    if output is None:
        for line in lines:
            print(line)
    else:
        with open(output, 'a') as f:
            for line in lines:
                f.write(line + '\n')

def main():
    description = "A simple script to add CTest ADD_TEST macros to a "+\
        "file for every test in a google-test executable." 
    parser = ap.ArgumentParser(description=description)

    executable = 'the path to the test exectuable to call'
    parser.add_argument('--executable', help=executable, required=True)

    output = "the file to write the ADD_TEST macros to "+\
        "(nominally CTestTestfile.cmake)"
    parser.add_argument('--output', help=output, required=True)

    args = parser.parse_args()

    assert os.path.exists(args.executable)
    assert os.path.exists(args.output)

    rtn = subprocess.Popen([args.executable, "--gtest_list_tests"], 
                           stdout=subprocess.PIPE, shell=(os.name=='nt'))
    
    tests = parse_tests(rtn.stdout.readlines())
    rtn.stdout.close()

    write_macros_to_output(tests, args.executable, args.output)

    return 0

if __name__ == "__main__":
    sys.exit(main())
