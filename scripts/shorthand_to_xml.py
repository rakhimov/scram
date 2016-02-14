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
3. Undefined nodes are processed as 'events' to the final XML output.
   Only warnings are emitted in the case of undefined nodes.
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

from collections import deque
import os
import re

import argparse as ap

from fault_tree import Node, BasicEvent, HouseEvent


class ParsingError(Exception):
    """General parsing errors."""

    pass


class FormatError(Exception):
    """Common errors in writing the shorthand format."""

    pass


class FaultTreeError(Exception):
    """Indication of problems in the fault tree."""

    pass


class Gate(Node):
    """Representation of a fault tree gate.

    Attributes:
        operator: Logical operator of this formula.
        k_num: Min number for the combination operator.
        node_arguments: String names of non-formula arguments like basic events.
        g_arguments: arguments that are gates.
        b_arguments: arguments that are basic events.
        h_arguments: arguments that are house events.
        u_arguments: arguments that are undefined from the input.
        mark: Marking for various algorithms like toposort.
    """

    def __init__(self, name, operator=None, k_num=None):
        """Initializes a gate.

        Args:
            name: Identifier of the node.
            operator: Boolean operator of this formula.
            k_num: Min number for the combination operator.
        """
        super(Gate, self).__init__(name)
        self.mark = None
        self.operator = operator
        self.k_num = k_num
        self.node_arguments = []
        self.g_arguments = []
        self.b_arguments = []
        self.h_arguments = []
        self.u_arguments = []

    def num_arguments(self):
        """Returns the number of arguments."""
        return len(self.node_arguments)


class FaultTree(object):
    """Representation of a fault tree for shorthand to XML purposes.

    Attributes:
        name: The name of a fault tree.
        gates: A collection of gates of a fault tree.
        basic_events: A collection of basic events of a fault tree.
        house_events: A collection of house events of a fault tree.
        undef_nodes: Nodes that are not explicitly defined as gates or
            basic events.
        top_gates: The top gates of a fault tree. Single one is the default.
        multi_top: A flag to indicate to allow multiple top gates.
    """

    def __init__(self, name=None, multi_top=False):
        """Initializes an empty fault tree.

        Args:
            name: The name of the system described by the fault tree container.
            multi_top: A flag to indicate multi-rooted container.
        """
        self.name = name
        self.gates = {}
        self.basic_events = {}
        self.house_events = {}
        self.undef_nodes = {}
        self.top_gates = None
        self.multi_top = multi_top

    def __check_redefinition(self, name):
        """Checks if a node is being redefined.

        Args:
            name: The name under investigation.

        Raises:
            FaultTreeError: The given name already exists.
        """
        if (name.lower() in self.basic_events or
                name.lower() in self.gates or
                name.lower() in self.house_events):
            raise FaultTreeError("Redefinition of a node: " + name)

    def __detect_cycle(self):
        """Checks if the fault tree has a cycle.

        Raises:
            FaultTreeError: There is a cycle in the fault tree.
        """
        def visit(gate):
            """Recursively visits the given gate sub-tree to detect a cycle.

            Upon visiting the descendant gates,
            their marks are changed from temporary to permanent.

            Args:
                gate: The current gate.

            Returns:
                None if no cycle is found.
                A list of node names in a detected cycle path in reverse order.
            """
            if not gate.mark:
                gate.mark = "temp"
                for child in gate.g_arguments:
                    cycle = visit(child)
                    if cycle:
                        cycle.append(gate.name)
                        return cycle
                gate.mark = "perm"
            elif gate.mark == "temp":
                return [gate.name]  # a cycle is detected
            return None  # the permanent mark

        def print_cycle(cycle):
            """Prints the detected cycle to the error.

            Args:
                cycle: A list of gate names in the cycle path in reverse order.

            Raises:
                FaultTreeError: There is a cycle in the fault tree.
            """
            start = cycle[0]
            cycle.reverse()  # print top-down
            raise FaultTreeError("Detected a cycle: " +
                                 "->".join(cycle[cycle.index(start):]))

        assert self.top_gates is not None
        for top_gate in self.top_gates:
            cycle = visit(top_gate)
            if cycle:
                print_cycle(cycle)

        detached_gates = [x for x in self.gates.values() if not x.mark]
        if detached_gates:
            error_msg = "Detected detached gates that may be in a cycle\n"
            error_msg += str([x.name for x in detached_gates])
            try:
                for gate in detached_gates:
                    cycle = visit(gate)
                    if cycle:
                        print_cycle(cycle)
            except FaultTreeError as error:
                error_msg += "\n" + str(error)
                raise FaultTreeError(error_msg)

    def __detect_top(self):
        """Detects the top gate of the developed fault tree.

        Raises:
            FaultTreeError: Multiple or no top gates are detected.
        """
        top_gates = [x for x in self.gates.values() if x.is_orphan()]
        if len(top_gates) > 1 and not self.multi_top:
            names = [x.name for x in top_gates]
            raise FaultTreeError("Detected multiple top gates:\n" + str(names))
        elif not top_gates:
            raise FaultTreeError("No top gate is detected")
        self.top_gates = top_gates

    def add_basic(self, name, prob):
        """Creates and adds a new basic event into the fault tree.

        Args:
            name: A name for the new basic event.
            prob: The probability of the new basic event.

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        self.basic_events.update({name.lower(): BasicEvent(name, prob)})

    def add_house(self, name, state):
        """Creates and adds a new house event into the fault tree.

        Args:
            name: A name for the new house event.
            state: The state of the new house event ("true" or "false").

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        self.house_events.update({name.lower(): HouseEvent(name, state)})

    def add_gate(self, name, operator, arguments, k_num=None):
        """Creates and adds a new gate into the fault tree.

        Args:
            name: A name for the new gate.
            operator: A gate operator for the new gate.
            arguments: Collection of argument node names of the new gate.
            k_num: K number is required for a combination type of a gate.

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        gate = Gate(name, operator, k_num)
        gate.node_arguments = arguments
        self.gates.update({name.lower(): gate})

    def populate(self):
        """Assigns arguments to gates and parents to arguments.

        Raises:
            FaultTreeError: There are problems with the fault tree.
        """
        for gate in self.gates.values():
            assert gate.num_arguments() > 0
            for child in gate.node_arguments:
                child_node = None
                if child.lower() in self.gates:
                    child_node = self.gates[child.lower()]
                    gate.g_arguments.append(child_node)
                elif child.lower() in self.basic_events:
                    child_node = self.basic_events[child.lower()]
                    gate.b_arguments.append(child_node)
                elif child.lower() in self.house_events:
                    child_node = self.house_events[child.lower()]
                    gate.h_arguments.append(child_node)
                elif child.lower() in self.undef_nodes:
                    child_node = self.undef_nodes[child.lower()]
                    gate.u_arguments.append(child_node)
                else:
                    print("Warning. Unidentified node: " + child)
                    child_node = Node(child)
                    self.undef_nodes.update({child.lower(): child_node})
                    gate.u_arguments.append(child_node)
                child_node.add_parent(gate)

        for basic in self.basic_events.values():
            if basic.is_orphan():
                print("Warning. Orphan basic event: " + basic.name)
        for house in self.house_events.values():
            if house.is_orphan():
                print("Warning. Orphan house event: " + house.name)
        self.__detect_top()
        self.__detect_cycle()


def parse_input_file(input_file, multi_top=False):
    """Parses an input file with a shorthand description of a fault tree.

    Args:
        input_file: The path to the input file.
        multi_top: If the input contains a fault tree with multiple top gates.

    Returns:
        The fault tree described in the input file.

    Raises:
        ParsingError: Parsing is unsuccessful.
        FormatError: Formatting problems in the input.
        FaultTreeError: Problems in the structure of the fault tree.
    """
    # Pattern for names and references
    name_sig = r"[a-zA-Z]\w*(-\w+)*"
    literal = r"~?" + name_sig
    # Fault tree name
    ft_name_re = re.compile(r"^(" + name_sig + r")$")
    # Probability description for a basic event
    prob_re = re.compile(r"^p\(\s*(?P<name>" + name_sig +
                         r")\s*\)\s*=\s*(?P<prob>1|0|0\.\d+)$")
    # State description for a house event
    state_re = re.compile(r"^s\(\s*(?P<name>" + name_sig +
                          r")\s*\)\s*=\s*(?P<state>true|false)$")
    # General gate name and pattern
    gate_sig = r"^(?P<name>" + name_sig + r")\s*:=\s*"
    gate_re = re.compile(gate_sig + r"(?P<formula>.+)$")
    # Optional parentheses for formulas
    paren_re = re.compile(r"\(([^()]+)\)$")
    # Gate type identification
    and_re = re.compile(r"(" + literal + r"(\s*&\s*" + literal + r"\s*)+)$")
    or_re = re.compile(r"(" + literal + r"(\s*\|\s*" + literal + r"\s*)+)$")
    vote_args = r"\[(\s*" + literal + r"(\s*,\s*" + literal + r"\s*){2,})\]"
    vote_re = re.compile(r"@\(\s*([2-9])\s*,\s*" + vote_args + r"\s*\)\s*$")
    xor_re = re.compile(r"(" + literal + r"\s*\^\s*" + literal + r")$")
    not_re = re.compile(r"~(" + name_sig + r")$")
    null_re = re.compile(r"(" + name_sig + r")$")

    ft_name = None
    fault_tree = FaultTree()

    def get_arguments(arguments_string, splitter):
        """Splits the input string into arguments of a formula.

        Args:
            arguments_string: String contaning arguments.
            splitter: Splitter specific to the operator, i.e. "&", "|", ','.

        Returns:
            arguments list from the input string.

        Raises:
            FaultTreeError: Repeated arguments for the node.
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
        if paren_re.match(line):
            line = paren_re.match(line).group(1).strip()
        arguments = None
        operator = None
        k_num = None
        if or_re.match(line):
            arguments = or_re.match(line).group(1)
            arguments = get_arguments(arguments, "|")
            operator = "or"
        elif xor_re.match(line):
            arguments = xor_re.match(line).group(1)
            arguments = get_arguments(arguments, "^")
            operator = "xor"
        elif and_re.match(line):
            arguments = and_re.match(line).group(1)
            arguments = get_arguments(arguments, "&")
            operator = "and"
        elif vote_re.match(line):
            k_num, arguments = vote_re.match(line).group(1, 2)
            arguments = get_arguments(arguments, ",")
            if int(k_num) >= len(arguments):
                raise FaultTreeError(
                    "Invalid k/n for the combination formula:\n" + line)
            operator = "atleast"
        elif not_re.match(line):
            arguments = not_re.match(line).group(1)
            arguments = get_arguments(arguments, "~")
            operator = "not"
        elif null_re.match(line):
            arguments = null_re.match(line).group(1)
            arguments = [arguments.strip()]
            operator = "null"  # pass-through
        else:
            raise ParsingError("Cannot interpret the formula:\n" + line)
        return operator, arguments, k_num

    line_num = 0
    shorthand_file = open(input_file, "r")
    for line in shorthand_file:
        line_num += 1
        line = line.strip()
        if not line:
            continue
        try:
            if gate_re.match(line):
                gate_name, formula_line = gate_re.match(line).group("name",
                                                                    "formula")
                operator, arguments, k_num = get_formula(formula_line)
                fault_tree.add_gate(gate_name, operator, arguments, k_num)
            elif prob_re.match(line):
                event_name, prob = prob_re.match(line).group("name", "prob")
                fault_tree.add_basic(event_name, prob)
            elif state_re.match(line):
                event_name, state = state_re.match(line).group("name", "state")
                fault_tree.add_house(event_name, state)
            elif ft_name_re.match(line):
                if ft_name:
                    raise FormatError(
                        "Redefinition of the fault tree name:\n%s to %s" %
                        (ft_name, line))
                ft_name = ft_name_re.match(line).group(1)
            else:
                raise ParsingError("Cannot interpret the line.")
        except ParsingError as err:
            raise ParsingError(str(err) + "\nIn line %d:\n" % line_num + line)
        except FormatError as err:
            raise FormatError(str(err) + "\nIn line %d:\n" % line_num + line)
        except FaultTreeError as err:
            raise FaultTreeError(str(err) + "\nIn line %d:\n" % line_num + line)
    if ft_name is None:
        raise FormatError("The fault tree name is not given.")
    fault_tree.name = ft_name
    fault_tree.multi_top = multi_top
    fault_tree.populate()
    return fault_tree


def toposort_gates(top_gates, gates):
    """Sorts gates topologically starting from the root gate.

    Args:
        top_gates: The top gates of the fault tree.
        gates: Gates to be sorted.

    Returns:
        A deque of sorted gates.
    """
    for gate in gates:
        gate.mark = ""

    def visit(gate, final_list):
        """Recursively visits the given gate sub-tree to include into the list.

        Args:
            gate: The current gate.
            final_list: A deque of sorted gates.
        """
        assert gate.mark != "temp"
        if not gate.mark:
            gate.mark = "temp"
            for arg in gate.g_arguments:
                visit(arg, final_list)
            gate.mark = "perm"
            final_list.appendleft(gate)

    sorted_gates = deque()
    for top_gate in top_gates:
        visit(top_gate, sorted_gates)
    assert len(sorted_gates) == len(gates)
    return sorted_gates


def write_to_xml_file(fault_tree, output_file):
    """Writes the fault tree into an XML file.

    The file is formatted according to OpenPSA MEF
    but without indentations for human readability.

    Args:
        fault_tree: A full fault tree.
        output_file: The output destination.
    """
    t_file = open(output_file, "w")
    t_file.write("<?xml version=\"1.0\"?>\n")
    t_file.write("<opsa-mef>\n")
    t_file.write("<define-fault-tree name=\"%s\">\n" % fault_tree.name)

    def write_gate(gate, o_file):
        """Write the gate in the OpenPSA MEF XML.

        Args:
            gate: The gate to be printed.
            o_file: The output file stream.
        """
        o_file.write("<define-gate name=\"" + gate.name + "\">\n")
        if gate.operator != "null":
            o_file.write("<" + gate.operator)
            if gate.operator == "atleast":
                o_file.write(" min=\"" + gate.k_num + "\"")
            o_file.write(">\n")
        # Print house events
        for h_child in gate.h_arguments:
            o_file.write("<house-event name=\"" + h_child.name + "\"/>\n")

        # Print basic events
        for b_child in gate.b_arguments:
            o_file.write("<basic-event name=\"" + b_child.name + "\"/>\n")

        # Print undefined events
        for u_child in gate.u_arguments:
            o_file.write("<event name=\"" + u_child.name + "\"/>\n")

        # Print gates
        for g_child in gate.g_arguments:
            o_file.write("<gate name=\"" + g_child.name + "\"/>\n")

        if gate.operator != "null":
            o_file.write("</" + gate.operator + ">\n")
        o_file.write("</define-gate>\n")

    sorted_gates = toposort_gates(fault_tree.top_gates,
                                  fault_tree.gates.values())
    for gate in sorted_gates:
        write_gate(gate, t_file)

    t_file.write("</define-fault-tree>\n")

    if fault_tree.basic_events or fault_tree.house_events:
        t_file.write("<model-data>\n")
        for basic_event in fault_tree.basic_events.values():
            t_file.write(basic_event.to_xml())

        for house_event in fault_tree.house_events.values():
            t_file.write(house_event.to_xml())
        t_file.write("</model-data>\n")

    t_file.write("</opsa-mef>")
    t_file.close()


def main():
    """Verifies arguments and calls parser and writer.

    Raises:
        ArgumentTypeError: There are problemns with the arguments.
    """
    description = "Shorthand => OpenPSA MEF XML Converter"

    parser = ap.ArgumentParser(description=description)

    input_file = "input file with the shorthand notation"
    parser.add_argument("input_file", type=str, nargs="?", help=input_file)

    multi_top = "multiple top events"
    parser.add_argument("--multi-top", help=multi_top, action="store_true")

    out = "output file to write the converted input"
    parser.add_argument("-o", "--out", help=out)

    args = parser.parse_args()

    if not args.input_file:
        raise ap.ArgumentTypeError("No input file is provided.")

    fault_tree = parse_input_file(args.input_file, args.multi_top)
    out = args.out
    if not out:
        out = os.path.basename(args.input_file)
        out = out[:out.rfind(".")] + ".xml"
    write_to_xml_file(fault_tree, out)

if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as arg_error:
        print("Argument Error:")
        print(arg_error)
    except ParsingError as parsing_error:
        print("Parsing Error:")
        print(parsing_error)
    except FormatError as format_error:
        print("Format Error:")
        print(format_error)
    except FaultTreeError as fault_tree_error:
        print("Error in the fault tree:")
        print(fault_tree_error)
