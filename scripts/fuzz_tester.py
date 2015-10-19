#!/usr/bin/env python
#
# Copyright (C) 2015 Olzhas Rakhimov
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

"""fuzz_tester.py

Runs SCRAM with various input files and configurations.
"""

from __future__ import print_function

import random
from subprocess import call
import sys

import argparse as ap


class Config(object):
    """Storage for configurations.

    Empty strings mean the default options of SCRAM.
    """
    switch = [ "--probability", "--importance"]
    approximation = ["", "--rare-event", "--mcub"]
    analysis = ["", "--bdd"]
    max_limit = 10

    @staticmethod
    def restrict():
        """Restricts configurations for testing preprocessor."""
        Config.switch = []
        Config.approximation = [""]
        Config.analysis = [""]
        Config.max_limit = 1


def generate_input():
    """Calls fault tree generator."""
    cmd = ["./fault_tree_generator.py",
           "-b", "100", "--common-b", "0.4", "--parents-b", "5",
           "--common-g", "0.2", "--parents-g", "3", "--children", "2.5",
           "--seed", str(random.randint(1, 1e8)),
           "--maxprob", "0.5", "--minprob", "0.1"]
    weights = ["--weights-g", "1", "1", "1"]
    if random.choice([True, False]):
        weights += ["0.01", "0.1"]  # Add non-coherence
    cmd += weights
    call(cmd)

def get_limit_order():
    """Generates limit on cut set order."""
    return random.randint(1, Config.max_limit)

def call_scram():
    """Calls SCRAM with generated input files."""
    cmd = ["scram", "fault_tree.xml"] + \
          ["--limit-order", str(get_limit_order())]

    if Config.switch:
        cmd += [random.choice(Config.switch), random.choice(["true", "false"])]

    approx = random.choice(Config.approximation)
    if approx:
        cmd.append(approx)

    algorithm = random.choice(Config.analysis)
    if algorithm:
        cmd.append(algorithm)
    print(cmd)
    cmd += ["-o", "/dev/null"]
    return call(cmd)

def main():
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

    args = parser.parse_args()

    if call(["which", "scram"]):
        print("SCRAM is not found in the PATH.")
        return 1

    if args.preprocessor:
        print("Focusing on Preprocessor")
        Config.restrict()
    elif args.mocus:
        print("Focusing on MOCUS")
        Config.analysis = [""]
    elif args.bdd:
        print("Focusing on BDD")
        Config.analysis = ["--bdd"]

    for i in range(args.num_runs):
        generate_input()
        if call_scram():
            print("SCRAM failed!")
            return 1
        if not (i + 1) % 100:
            print("Finished run #" + str(i + 1))

    return 0

if __name__ == "__main__":
    sys.exit(main())
