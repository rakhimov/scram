#!/usr/bin/env python
#
# Copyright (C) 2015-2016 Olzhas Rakhimov
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Runs SCRAM with various input files and configurations.

This script is helpful to detect rare bugs, failed assumptions,
flawed design, and bottlenecks.
For the full functionality of the Fuzz tester,
SCRAM must be built in non-Release mode
so that NDEBUG is undefined.

In addition to its main purpose,
the script is handy to discover
complex auto-generated analysis inputs and configurations.
"""

import hashlib
import logging
import multiprocessing
import os
import random
import resource
from subprocess import call
import sys
from tempfile import NamedTemporaryFile

import argparse as ap

import fault_tree_generator as ft_gen


class Config(object):
    """Storage for configurations.

    Empty strings mean the default options of SCRAM.

    Attributes:
        switch: SCRAM flags that take true or false values.
        approximation: SCRAM quantitative analysis approximations.
        analysis: Qualitative analysis algorithms.
        max_limit: The largest size limit on the cut sets.
        additional: A list of commands to be appended without fuzzing.
    """

    switch = ["--probability", "--importance"]
    approximation = ["", "--rare-event", "--mcub"]
    analysis = ["--mocus", "--bdd", "--zbdd"]
    max_limit = 10
    additional = []

    @staticmethod
    def restrict():
        """Restricts configurations for testing preprocessor."""
        Config.switch = []
        Config.approximation = [""]
        Config.additional.append("--preprocessor")

    @staticmethod
    def configure(args):
        """Adjusts configurations with the cmd-line arguments."""
        if args.cross_validate:
            logging.info("Cross validating algorithms")
            Config.analysis = [""]
            Config.switch = []
            Config.approximation = [""]
            return

        if args.prime_implicants:
            logging.info("Focusing on Prime Implicants")
            Config.analysis = ["--bdd"]
            Config.approximation = [""]
            Config.additional.append("--prime-implicants")
        elif args.mocus:
            logging.info("Focusing on MOCUS")
            Config.analysis = ["--mocus"]
        elif args.bdd:
            logging.info("Focusing on BDD")
            Config.analysis = ["--bdd"]
        elif args.zbdd:
            logging.info("Focusing on ZBDD")
            Config.analysis = ["--zbdd"]

        if args.preprocessor:
            logging.info("Focusing on Preprocessor")
            Config.restrict()


def generate_input(normal, coherent, output_dir=None):
    """Calls fault tree generator.

    The auto-generated input file is located in the run-time directory
    with the default name given by the fault tree generator.

    Args:
        normal: Flag for models with AND/OR gates only.
        coherent: Flag for generation of coherent models.
        output_dir: The directory to put the generated input file.

    Returns:
        The path to the input file.
    """
    input_file = NamedTemporaryFile(dir=output_dir, prefix="fault_tree_",
                                    suffix=".xml", delete=False)
    cmd = ["--num-basic", "100", "--common-b", "0.4", "--parents-b", "5",
           "--common-g", "0.2", "--parents-g", "3", "--num-args", "2.5",
           "--seed", str(random.randint(1, 1e8)),
           "--max-prob", "0.5", "--min-prob", "0.1",
           "-o", input_file.name]
    weights = ["--weights-g", "1", "1"]
    if not normal:
        weights += ["1"]
        if not coherent and random.choice([True, False]):
            weights += ["0.01", "0.1"]  # Add non-coherence
    cmd += weights
    ft_gen.main(cmd)
    return input_file.name


def get_log_file_name(input_file):
    """Creates a unique log file name.

    Args:
        input_file: The name of input file with ".xml" suffix.

    Returns:
        The name of the log file with ".log" suffix.
    """
    return input_file[:input_file.rfind(".")] + ".log"


def get_limit_order():
    """Generates the size limit on cut set order.

    Returns:
        Random integer between 1 and the maximum limit on cut set order.
    """
    return random.randint(1, Config.max_limit)


def generate_analysis_call(input_file):
    """Generates the analysis call.

    Args:
        input_file: The path to the input file.

    Returns:
        An array to pass to the call command.
    """
    cmd = ["scram", input_file, "--limit-order", str(get_limit_order())]
    if Config.switch:
        cmd += [random.choice(Config.switch), random.choice(["true", "false"])]
    approx = random.choice(Config.approximation)
    if approx:
        cmd.append(approx)
    analysis = random.choice(Config.analysis)
    if analysis:
        cmd.append(analysis)
    cmd += Config.additional
    return cmd


def call_scram(input_file):
    """Calls SCRAM with generated input files.

    The logs with the call signature and report
    are placed in a file with the name of the input file but "log" suffix.

    Args:
        input_file: The path to the input file.

    Returns:
        0 for successful runs.
    """
    cmd = generate_analysis_call(input_file)
    logging.info(cmd)
    cmd += ["--verbosity", "5", "-o", "/dev/null"]
    log_file = open(get_log_file_name(input_file), "w")
    log_file.write(str(cmd) + "\n")
    log_file.flush()
    ret = call(cmd, stderr=log_file)
    log_file.write("SCRAM run return value: " + str(ret) + "\n")
    return ret


def cross_validate(input_file):
    """Calls all SCRAM algorithms with generated input files.

    The logs with the call signature and report
    are placed in a file with the name of the input file but "log" suffix.

    Args:
        input_file: The path to the input file.

    Returns:
        0 for successful runs.
        1 for failed runs.
    """
    cmd = generate_analysis_call(input_file)
    logging.info(cmd)
    cmd += ["--print"]
    log_file = open(get_log_file_name(input_file), "w")
    log_file.write(str(cmd) + "\n")

    def check_algorithm(flag):
        """Runs SCRAM with the given algorithm flag."""
        out_file = NamedTemporaryFile()
        ret = call(cmd + [flag], stderr=out_file)
        if ret:
            log_file.write("SCRAM run return value: " + str(ret) + "\n")
            return None, None
        with open(out_file.name) as report:
            return report.readline(), hashlib.md5(report.read()).hexdigest()

    result = set()

    def get_result(algorithm):
        """Gets the results of running the algorithm."""
        log_file.write("Running " + algorithm.upper() + ":\n")
        summary, md5_hash = check_algorithm("--" + algorithm.lower())
        if not summary:
            return 1
        log_file.write(" Summary: " + summary)  # Newline is in 'summary'.
        log_file.write(" Hash: " + str(md5_hash) + "\n")
        result.add((summary, md5_hash))
        return 0

    if get_result("bdd") | get_result("zbdd") | get_result("mocus"):
        log_file.write("Analysis failed!\n")
        return 1

    assert result
    if len(result) > 1:
        log_file.write("Disagreement between algorithms!\n")
        return 1

    return 0


def main():
    """The main entrance for SCRAM Fuzz Tester.

    Returns:
        0 if tests finished without failures.
        1 for test failures.
        2 for system failures.
    """
    parser = ap.ArgumentParser(description="SCRAM Fuzz Tester")
    parser.add_argument("-n", "--num-runs", type=int, help="# of SCRAM runs",
                        default=10, metavar="int")
    parser.add_argument("--preprocessor", action="store_true",
                        help="focus on Preprocessor")
    parser.add_argument("--mocus", action="store_true", help="focus on MOCUS")
    parser.add_argument("--bdd", action="store_true", help="focus on BDD")
    parser.add_argument("--zbdd", action="store_true", help="focus on ZBDD")
    parser.add_argument("--cross-validate", action="store_true",
                        help="compare results of BDD, ZBDD, and MOCUS")
    parser.add_argument("--coherent", action="store_true",
                        help="focus on coherent models")
    parser.add_argument("--normal", action="store_true",
                        help="focus on models with AND/OR gates only")
    parser.add_argument("--prime-implicants", action="store_true",
                        help="focus on Prime Implicants")
    parser.add_argument("--time-limit", type=int, metavar="seconds",
                        help="CPU time limit for each run")
    parser.add_argument("-j", "--jobs", type=int, metavar="N",
                        help="allow N runs (jobs) at once")
    parser.add_argument("-o", "--output-dir", type=str, metavar="path",
                        help="directory to put results")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    if call(["which", "scram"]):
        logging.error("SCRAM is not found in the PATH.")
        return 2
    if args.output_dir and not os.path.isdir(args.output_dir):
        logging.error("The output directory doesn't exist.")
        return 2

    if args.time_limit:
        resource.setrlimit(resource.RLIMIT_CPU,
                           (args.time_limit, args.time_limit))

    Config.configure(args)
    return any(list(get_map(args.jobs)(Fuzzer(args), range(args.num_runs))))


def get_map(working_threads):
    """Returns the map method for jobs."""
    if working_threads <= 1:
        return map
    return multiprocessing.Pool(processes=working_threads).imap_unordered


class Fuzzer(object):  # pylint: disable=too-few-public-methods
    """Runs fuzz testing."""

    def __init__(self, args):
        """Saves the argument for later use."""
        self.args = args
        self.call_function = (cross_validate
                              if args.cross_validate else call_scram)

    def __call__(self, job_number):
        """Returns 1 for failure and 0 for success."""
        input_file = generate_input(self.args.normal, self.args.coherent,
                                    self.args.output_dir)
        num_runs = job_number + 1
        if self.call_function(input_file):
            logging.info("SCRAM failed run " + str(num_runs) + ": " +
                         input_file)
            return 1
        os.remove(input_file)
        os.remove(get_log_file_name(input_file))
        if not num_runs % 10:
            logging.info(
                "\n========== Finished run #" + str(num_runs) + " ==========\n")
        return 0


if __name__ == "__main__":
    sys.exit(main())
