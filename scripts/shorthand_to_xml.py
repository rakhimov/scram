#!/usr/bin/env python
#
# Copyright (C) 2014-2016 Olzhas Rakhimov
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

"""Converts the shorthand notation for fault trees into an XML file.

The output file is formatted according to the OpenPSA MEF.
The default output file name is the input file name with the XML extension.

The shorthand notation is described as follows:
AND gate:                          gate_name := (arg1 & arg2 & ...)
OR gate:                           gate_name := (arg1 | arg2 | ...)
ATLEAST(k/n) gate:                 gate_name := @(k, [arg1, arg2, ...])
NOT gate:                          gate_name := ~arg
XOR gate:                          gate_name := (arg1 ^ arg2)
NULL gate:                         gate_name := arg
Probability of a basic event:      p(event_name) = probability
Boolean state of a house event:    s(event_name) = state

Some requirements and additions to the shorthand format:
1. The names and references are not case-sensitive
   and must be formatted according to 'XML NCNAME datatype'
   without double dashes('--'), period('.'), and trailing '-'.
2. Arguments can be complemented with '~'.
3. Undefined arguments are processed as 'events' to the final XML output.
   Only warnings are emitted in the case of undefined events.
4. Name clashes or redefinitions are errors.
5. Cycles in the graph are detected by the script as errors.
6. The top gate is detected by the script.
   Only one top gate is allowed
   unless otherwise specified by the user.
7. Repeated arguments are considered an error.
8. The script is flexible with whitespace characters in the input file.
9. Parentheses are optional for logical operators except for ATLEAST.
"""

from __future__ import print_function

import os
import re
import sys

import argparse as ap

from fault_tree import Event, BasicEvent, HouseEvent, Gate, FaultTree


class ParsingError(Exception):
    """General parsing errors."""

    pass


class FormatError(Exception):
    """Common errors in writing the shorthand format."""

    pass


class FaultTreeError(Exception):
    """Indication of problems in the fault tree."""

    pass


class LateBindingGate(Gate):
    """Representation of a gate with arguments defined late or not at all.

    Attributes:
        event_arguments: String names of arguments.
    """

    def __init__(self, name, operator=None, k_num=None):
        """Initializes a gate.

        Args:
            name: Identifier of the event.
            operator: Boolean operator of this formula.
            k_num: Min number for the combination operator.
        """
        super(LateBindingGate, self).__init__(name, operator, k_num)
        self.event_arguments = []

    def num_arguments(self):
        """Returns the number of arguments."""
        return len(self.event_arguments)


class LateBindingFaultTree(FaultTree):
    """Representation of a fault tree for shorthand to XML purposes.

    Attributes:
        multi_top: A flag to indicate to allow multiple top gates.
    """

    def __init__(self, name=None, multi_top=False):
        """Initializes an empty fault tree.

        Args:
            name: The name of the system described by the fault tree container.
            multi_top: A flag to indicate multi-rooted container.
        """
        super(LateBindingFaultTree, self).__init__(name)
        self.multi_top = multi_top
        self.__gates = {}
        self.__basic_events = {}
        self.__house_events = {}
        self.__undef_events = {}

    def __check_redefinition(self, name):
        """Checks if an event is being redefined.

        Args:
            name: The name under investigation.

        Raises:
            FaultTreeError: The given name already exists.
        """
        if (name.lower() in self.__basic_events or
                name.lower() in self.__gates or
                name.lower() in self.__house_events):
            raise FaultTreeError("Redefinition of an event: " + name)

    def __visit(self, gate):
        """Recursively visits the given gate sub-tree to detect a cycle.

        Upon visiting the descendant gates,
        their marks are changed from temporary to permanent.

        Args:
            gate: The current gate.

        Returns:
            None if no cycle is found.
            A list of event names in a detected cycle path in reverse order.
        """
        if not gate.mark:
            gate.mark = "temp"
            for child in gate.g_arguments:
                cycle = self.__visit(child)
                if cycle:
                    cycle.append(gate.name)
                    return cycle
            gate.mark = "perm"
        elif gate.mark == "temp":
            return [gate.name]  # a cycle is detected
        return None  # the permanent mark

    def __raise_cycle(self, cycle):
        """Prints the detected cycle with the error.

        Args:
            cycle: A list of gate names in the cycle path in reverse order.

        Raises:
            FaultTreeError: Error with a message containing the cycle.
        """
        start = cycle[0]
        cycle.reverse()  # print top-down
        raise FaultTreeError("Detected a cycle: " +
                             "->".join(cycle[cycle.index(start):]))

    def __detect_cycle(self):
        """Checks if the fault tree has a cycle.

        Raises:
            FaultTreeError: There is a cycle in the fault tree.
        """
        assert self.top_gates is not None
        for top_gate in self.top_gates:
            cycle = self.__visit(top_gate)
            if cycle:
                self.__raise_cycle(cycle)

        detached_gates = [x for x in self.gates if not x.mark]
        if detached_gates:
            error_msg = "Detected detached gates that may be in a cycle\n"
            error_msg += str([x.name for x in detached_gates])
            try:
                for gate in detached_gates:
                    cycle = self.__visit(gate)
                    if cycle:
                        self.__raise_cycle(cycle)
            except FaultTreeError as error:
                error_msg += "\n" + str(error)
                raise FaultTreeError(error_msg)

    def __detect_top(self):
        """Detects the top gate of the developed fault tree.

        Raises:
            FaultTreeError: Multiple or no top gates are detected.
        """
        top_gates = [x for x in self.gates if x.is_orphan()]
        if len(top_gates) > 1 and not self.multi_top:
            names = [x.name for x in top_gates]
            raise FaultTreeError("Detected multiple top gates:\n" + str(names))
        elif not top_gates:
            raise FaultTreeError("No top gate is detected")
        self.top_gates = top_gates

    def add_basic_event(self, name, prob):
        """Creates and adds a new basic event into the fault tree.

        Args:
            name: A name for the new basic event.
            prob: The probability of the new basic event.

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        event = BasicEvent(name, prob)
        self.__basic_events.update({name.lower(): event})
        self.basic_events.append(event)

    def add_house_event(self, name, state):
        """Creates and adds a new house event into the fault tree.

        Args:
            name: A name for the new house event.
            state: The state of the new house event ("true" or "false").

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        event = HouseEvent(name, state)
        self.__house_events.update({name.lower(): event})
        self.house_events.append(event)

    def add_gate(self, name, operator, arguments, k_num=None):
        """Creates and adds a new gate into the fault tree.

        Args:
            name: A name for the new gate.
            operator: A gate operator for the new gate.
            arguments: Collection of argument event names of the new gate.
            k_num: K number is required for a combination type of a gate.

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        gate = LateBindingGate(name, operator, k_num)
        gate.event_arguments = arguments
        self.__gates.update({name.lower(): gate})
        self.gates.append(gate)

    def populate(self):
        """Assigns arguments to gates and parents to arguments.

        Raises:
            FaultTreeError: There are problems with the fault tree.
        """
        for gate in self.gates:
            assert gate.num_arguments() > 0
            for event_name in gate.event_arguments:
                if event_name.lower() in self.__gates:
                    gate.add_argument(self.__gates[event_name.lower()])
                elif event_name.lower() in self.__basic_events:
                    gate.add_argument(self.__basic_events[event_name.lower()])
                elif event_name.lower() in self.__house_events:
                    gate.add_argument(self.__house_events[event_name.lower()])
                elif event_name.lower() in self.__undef_events:
                    gate.add_argument(self.__undef_events[event_name.lower()])
                else:
                    print("Warning. Unidentified event: " + event_name)
                    event_node = Event(event_name)
                    self.__undef_events.update({event_name.lower(): event_node})
                    gate.add_argument(event_node)

        for basic_event in self.basic_events:
            if basic_event.is_orphan():
                print("Warning. Orphan basic event: " + basic_event.name)
        for house_event in self.house_events:
            if house_event.is_orphan():
                print("Warning. Orphan house event: " + house_event.name)
        self.__detect_top()
        self.__detect_cycle()

    def undefined_events(self):
        """Returns list of undefined events."""
        return self.__undef_events.values()


# Pattern for names and references
_NAME_SIG = r"[a-zA-Z]\w*(-\w+)*"
_LITERAL = r"~?" + _NAME_SIG
_RE_FT_NAME = re.compile(r"^(" + _NAME_SIG + r")$")  # Fault tree name
# Probability description for a basic event
_RE_PROB = re.compile(r"^p\(\s*(?P<name>" + _NAME_SIG +
                      r")\s*\)\s*=\s*(?P<prob>1|0|0\.\d+)$")
# State description for a house event
_RE_STATE = re.compile(r"^s\(\s*(?P<name>" + _NAME_SIG +
                       r")\s*\)\s*=\s*(?P<state>true|false)$")
# General gate name and pattern
_GATE_SIG = r"^(?P<name>" + _NAME_SIG + r")\s*:=\s*"
_RE_GATE = re.compile(_GATE_SIG + r"(?P<formula>.+)$")
# Optional parentheses for formulas
_RE_PAREN = re.compile(r"\(([^()]+)\)$")
# Gate type identifications
_RE_AND = re.compile(r"(" + _LITERAL + r"(\s*&\s*" + _LITERAL + r"\s*)+)$")
_RE_OR = re.compile(r"(" + _LITERAL + r"(\s*\|\s*" + _LITERAL + r"\s*)+)$")
_VOTE_ARGS = r"\[(\s*" + _LITERAL + r"(\s*,\s*" + _LITERAL + r"\s*){2,})\]"
_RE_VOTE = re.compile(r"@\(\s*([2-9])\s*,\s*" + _VOTE_ARGS + r"\s*\)\s*$")
_RE_XOR = re.compile(r"(" + _LITERAL + r"\s*\^\s*" + _LITERAL + r")$")
_RE_NOT = re.compile(r"~(" + _NAME_SIG + r")$")
_RE_NULL = re.compile(r"(" + _NAME_SIG + r")$")


def get_arguments(arguments_string, splitter):
    """Splits the input string into arguments of a formula.

    Args:
        arguments_string: String contaning arguments.
        splitter: Splitter specific to the operator, i.e. "&", "|", ','.

    Returns:
        arguments list from the input string.

    Raises:
        FaultTreeError: Repeated arguments for the formula.
    """
    arguments = arguments_string.strip().split(splitter)
    arguments = [x.strip() for x in arguments]
    if len(arguments) > len(set([x.lower() for x in arguments])):
        raise FaultTreeError("Repeated arguments:\n" + arguments_string)
    return arguments


def get_formula(line):
    """Constructs formula from the given line.

    Args:
        line: A string containing a Boolean equation.

    Returns:
        A formula operator, arguments, and k_num.

    Raises:
        ParsingError: Parsing is unsuccessful.
        FormatError: Formatting problems in the input.
        FaultTreeError: Problems in the structure of the formula.
    """
    line = line.strip()
    if _RE_PAREN.match(line):
        line = _RE_PAREN.match(line).group(1).strip()
    arguments = None
    operator = None
    k_num = None
    if _RE_OR.match(line):
        arguments = _RE_OR.match(line).group(1)
        arguments = get_arguments(arguments, "|")
        operator = "or"
    elif _RE_XOR.match(line):
        arguments = _RE_XOR.match(line).group(1)
        arguments = get_arguments(arguments, "^")
        operator = "xor"
    elif _RE_AND.match(line):
        arguments = _RE_AND.match(line).group(1)
        arguments = get_arguments(arguments, "&")
        operator = "and"
    elif _RE_VOTE.match(line):
        k_num, arguments = _RE_VOTE.match(line).group(1, 2)
        arguments = get_arguments(arguments, ",")
        if int(k_num) >= len(arguments):
            raise FaultTreeError(
                "Invalid k/n for the combination formula:\n" + line)
        operator = "atleast"
    elif _RE_NOT.match(line):
        arguments = _RE_NOT.match(line).group(1)
        arguments = get_arguments(arguments, "~")
        operator = "not"
    elif _RE_NULL.match(line):
        arguments = _RE_NULL.match(line).group(1)
        arguments = [arguments.strip()]
        operator = "null"  # pass-through
    else:
        raise ParsingError("Cannot interpret the formula:\n" + line)
    return operator, arguments, k_num


def interpret_line(line, fault_tree):
    """Interprets a line from the shorthand format input.

    Args:
        line: The line in the shorthand format.
        fault_tree: The fault tree container to update.

    Raises:
        ParsingError: Parsing is unsuccessful.
        FormatError: Formatting problems in the input.
        FaultTreeError: Problems in the structure of the fault tree.
    """
    line = line.strip()
    if not line:
        return
    if _RE_GATE.match(line):
        gate_name, formula_line = _RE_GATE.match(line).group("name", "formula")
        operator, arguments, k_num = get_formula(formula_line)
        fault_tree.add_gate(gate_name, operator, arguments, k_num)
    elif _RE_PROB.match(line):
        event_name, prob = _RE_PROB.match(line).group("name", "prob")
        fault_tree.add_basic_event(event_name, prob)
    elif _RE_STATE.match(line):
        event_name, state = _RE_STATE.match(line).group("name", "state")
        fault_tree.add_house_event(event_name, state)
    elif _RE_FT_NAME.match(line):
        if fault_tree.name:
            raise FormatError("Redefinition of the fault tree name:\n%s to %s" %
                              (fault_tree.name, line))
        fault_tree.name = _RE_FT_NAME.match(line).group(1)
    else:
        raise ParsingError("Cannot interpret the line.")


def parse_input(shorthand_file, multi_top=False):
    """Parses an input file with a shorthand description of a fault tree.

    Args:
        shorthand_file: The input file open for reads.
        multi_top: If the input contains a fault tree with multiple top gates.

    Returns:
        The fault tree described in the input file.

    Raises:
        ParsingError: Parsing is unsuccessful.
        FormatError: Formatting problems in the input.
        FaultTreeError: Problems in the structure of the fault tree.
    """
    fault_tree = LateBindingFaultTree()
    assert fault_tree.name is None
    line_num = 0
    line = None
    try:
        for line in shorthand_file:
            line_num += 1
            interpret_line(line, fault_tree)
    except ParsingError as err:
        raise ParsingError(str(err) + "\nIn line %d:\n" % line_num + line)
    except FormatError as err:
        raise FormatError(str(err) + "\nIn line %d:\n" % line_num + line)
    except FaultTreeError as err:
        raise FaultTreeError(str(err) + "\nIn line %d:\n" % line_num + line)
    if fault_tree.name is None:
        raise FormatError("The fault tree name is not given.")
    fault_tree.multi_top = multi_top
    fault_tree.populate()
    return fault_tree


def main(argv=None):
    """Verifies arguments and calls parser and writer.

    Args:
        argv: An optional list containing the command-line arguments.

    Raises:
        ArgumentTypeError: Problemns with the arguments.
        IOError: Input or output files are not accessible.
        ParsingError: Problems parsing the input file.
        FormatError: Formatting issues in the input.
        FaultTreeError: The input fault tree is malformed.
    """
    description = "Shorthand => OpenPSA MEF XML Converter"
    parser = ap.ArgumentParser(description=description)
    parser.add_argument("input_file", type=str,
                        help="input file with the shorthand notation")
    parser.add_argument("--multi-top", help="multiple top events",
                        action="store_true")
    parser.add_argument("-o", "--out",
                        help="output file to write the converted input")
    args = parser.parse_args(argv)

    fault_tree = None
    with open(args.input_file, "r") as shorthand_file:
        fault_tree = parse_input(shorthand_file, args.multi_top)

    out = args.out
    if not out:
        out = os.path.basename(args.input_file)
        out = out[:out.rfind(".")] + ".xml"

    with open(out, "w") as tree_file:
        tree_file.write("<?xml version=\"1.0\"?>\n")
        tree_file.write(fault_tree.to_xml())

if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as err:
        print("Argument Error:\n" + str(err))
        sys.exit(2)
    except IOError as err:
        print("IO Error:\n" + str(err))
        sys.exit(1)
    except ParsingError as err:
        print("Parsing Error:\n" + str(err))
        sys.exit(1)
    except FormatError as err:
        print("Format Error:\n" + str(err))
        sys.exit(1)
    except FaultTreeError as err:
        print("Error in the fault tree:\n" + str(err))
        sys.exit(1)
