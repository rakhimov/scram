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

switches = [ "--probability", "--importance"]
approximation = ["", "--rare-event", "--mcub"]
analysis = ["", "--bdd"]

def generate_input():
    """Calls fault tree generator."""
    cmd = ["./fault_tree_generator.py",
           "-b", "100", "--common-b", "0.4", "--parents-b", "5",
           "--common-g", "0.2", "--parents-g", "3", "--children", "2.5",
           "--seed", str(random.randint(1, 1e6)),
           "--maxprob", "0.5", "--minprob", "0.1"]
    weights = ["--weights-g", "1", "1", "1"]
    if random.choice([True, False]):
        weights += ["0.01", "0.1"]  # Add non-coherence
    cmd += weights
    call(cmd)

def get_limit_order():
    """Generates limit on cut set order."""
    return random.randint(1, 10)

def call_scram():
    """Calls SCRAM with generated input files."""
    cmd = ["scram", "fault_tree.xml", "-o", "/dev/null"] + \
          ["--limit-order", str(get_limit_order())] + \
          [random.choice(switches), random.choice(["true", "false"])]
    approx = random.choice(approximation)
    algorithm = random.choice(analysis)
    if approx:
        cmd += [approx]
    if algorithm:
        cmd += [algorithm]
    print(cmd)
    return call(cmd)

def main():
    for _ in range(10):
        generate_input()
        if call_scram():
            print("SCRAM failed!")
            break

if __name__ == "__main__":
    main()
