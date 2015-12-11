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

"""nqueens.py

This script generates a fault tree representation of the N Queens problem.
The representation is given in the shorthand format.
"""

from __future__ import print_function

import argparse as ap


def row(i):
    """Returns row position signature."""
    return "r" + str(i)

def col(j):
    """Returns column position signature."""
    return "c" + str(j)

def position(i, j, complement):
    """Produces a name for a literal position.

    Args:
        i: Row position.
        j: Column position.
        complement: Indication of a complement.

    Returns:
        A string "eij" with the complement indicated with "~" in the front.
    """
    assert i and j
    return ("~" if complement else "") + "Q" + row(i) + col(j)

def gate(i, j=None):
    """Produces a name for a gate logic for positions.

    Args:
        i: Row position.
        j: Optional column position.
    """
    return "G" + row(i) + (col(j) if j else "")


def main():
    """Prints the the N Queens fault tree representation to standard output."""
    description = "Fault tree representation of the N Queens problem"
    parser = ap.ArgumentParser(description=description)
    parser.add_argument("n", type=int, nargs="?", help="the number of queens",
                        default=8)
    args = parser.parse_args()

    if args.n < 1:
        raise ap.ArgumentTypeError("Illegal number of queens.")

    n = args.n
    print("NQueens" + str(n))

    # Getting the main setup constraints.
    for i in range(1, n + 1):
        for j in range(1, n + 1):
            logic = [position(i, j, False)]
            for k in range(1, n + 1):
                if k != j:
                    logic.append(position(i, k, True))
                if k != i:
                    logic.append(position(k, j, True))
                    diag_one = j + k - i
                    if diag_one > 0 and diag_one <= n:
                        logic.append(position(k, diag_one, True))
                    diag_two = j + i - k
                    if diag_two > 0 and diag_two <= n:
                        logic.append(position(k, diag_two, True))
            print(gate(i, j) + " := " + " & ".join(logic))

    # Getting the setup requirements.
    board = []  # top logic
    for i in range(1, n + 1):
        board.append(gate(i))
        row = []
        for j in range(1, n + 1):
            row.append(gate(i, j))
        print(gate(i) + " := " + " | ".join(row))

    print(gate(0) + " := " + " & ".join(board))

    # Provide probabilities for events.
    for i in range(1, n + 1):
        for j in range(1, n + 1):
            print("p(" + position(i, j, False) + ") = 1")


if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as error:
        print("Argument Error:")
        print(error)
