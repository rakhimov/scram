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

In addition to its main purpose,
the script is handy to discover
complex auto-generated analysis inputs and configurations.
"""

from __future__ import print_function

import random
from subprocess import call
import sys

import argparse as ap


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


def generate_input(normal, coherent):
    """Calls fault tree generator.

    The auto-generated input file is located in the run-time directory
    with the default name given by the fault tree generator.

    Args:
        normal: Flag for models with AND/OR gates only.
        coherent: Flag for generation of coherent models.
    """
    cmd = ["./fault_tree_generator.py",
           "-b", "100", "--common-b", "0.4", "--parents-b", "5",
           "--common-g", "0.2", "--parents-g", "3", "--children", "2.5",
           "--seed", str(random.randint(1, 1e8)),
           "--maxprob", "0.5", "--minprob", "0.1"]
    weights = ["--weights-g", "1", "1"]
    if not normal:
        weights += ["1"]
        if not coherent and random.choice([True, False]):
            weights += ["0.01", "0.1"]  # Add non-coherence
    cmd += weights
    call(cmd)


def get_limit_order():
    """Generates the size limit on cut set order.

    Returns:
        Random integer between 1 and the maximum limit on cut set order.
    """
    return random.randint(1, Config.max_limit)


def call_scram():
    """Calls SCRAM with generated input files.

    Returns:
        0 for successful runs.
    """
    cmd = ["scram", "fault_tree.xml"] + \
          ["--limit-order", str(get_limit_order())]

    if Config.switch:
        cmd += [random.choice(Config.switch), random.choice(["true", "false"])]

    approx = random.choice(Config.approximation)
    if approx:
        cmd.append(approx)

    cmd.append(random.choice(Config.analysis))
    cmd += Config.additional
    print(cmd)
    cmd += ["-o", "/dev/null"]
    return call(cmd)


def main():
    """The main entrance for SCRAM Fuzz Tester.

    Returns:
        0 if tests finished without failures.
        1 for failures.
    """
    description = "SCRAM Fuzz Tester"

    parser = ap.ArgumentParser(description=description)

    num_runs = "number of SCRAM calls"
    parser.add_argument("-n", "--num-runs", type=int, help=num_runs, default=10,
                        metavar="int")

    preprocessor = "focus on Preprocessor"
    parser.add_argument("--preprocessor", action="store_true",
                        help=preprocessor)

    mocus = "focus on MOCUS"
    parser.add_argument("--mocus", action="store_true", help=mocus)

    bdd = "focus on BDD"
    parser.add_argument("--bdd", action="store_true", help=bdd)

    zbdd = "focus on ZBDD"
    parser.add_argument("--zbdd", action="store_true", help=zbdd)

    coherent = "focus on coherent models"
    parser.add_argument("--coherent", action="store_true", help=coherent)

    normal = "focus on models only with AND/OR gates"
    parser.add_argument("--normal", action="store_true", help=normal)

    pi = "focus on Prime Implicants"
    parser.add_argument("--prime-implicants", action="store_true", help=pi)

    args = parser.parse_args()

    if call(["which", "scram"]):
        print("SCRAM is not found in the PATH.")
        return 1

    if args.prime_implicants:
        print("Focusing on Prime Implicants")
        Config.analysis = ["--bdd"]
        Config.approximation = [""]
        Config.additional.append("--prime-implicants")
    elif args.mocus:
        print("Focusing on MOCUS")
        Config.analysis = ["--mocus"]
    elif args.bdd:
        print("Focusing on BDD")
        Config.analysis = ["--bdd"]
    elif args.zbdd:
        print("Focusing on ZBDD")
        Config.analysis = ["--zbdd"]

    if args.preprocessor:
        print("Focusing on Preprocessor")
        Config.restrict()

    for i in range(args.num_runs):
        generate_input(args.normal, args.coherent)
        if call_scram():
            print("SCRAM failed!")
            return 1
        if not (i + 1) % 100:
            print("Finished run #" + str(i + 1))

    return 0

if __name__ == "__main__":
    sys.exit(main())
