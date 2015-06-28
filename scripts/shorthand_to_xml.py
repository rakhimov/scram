#!/usr/bin/env python
"""shorthand_to_xml.py

This script converts the shorthand notation for fault trees into an
XML file. The output file is formatted according to OpenPSA MEF.
The default output file name is the input file name with the XML extension.

The shorthand notation is described as follows:
AND gate:                          gate_name := (child1 & child2 & ...)
OR gate:                           gate_name := (child1 | child2 | ...)
ATLEAST(k/n) gate:                 gate_name := @(k, [child1, child2, ...])
NOT gate:                          gate_name := ~child
XOR gate:                          gate_name := (child1 ^ child2)
Probability of a basic event:      p(event_name) = probability
Boolean state of a house event:    s(event_name) = state

Some requirements and additions to the shorthand format:
1. The names and references are not case-sensitive and must be formatted
   according to 'XML NCNAME datatype' without double dashes('--'), period('.'),
   and trailing '-'.
2. No requirement for the structure of the input, i.e. topologically sorted.
3. Undefined nodes are processed as 'events' to the final XML output. However,
   warnings will be emitted in case it is the user's mistake.
4. Name clashes or redefinitions are errors.
5. Cyclic trees are detected by the script as errors.
6. The top gate is detected by the script. Only one top gate is allowed
   unless otherwise specified by the user.
7. Repeated children are considered an error.
8. The script is flexible with whitespace characters in the input file.
9. Parentheses are optional for logical operators except for ATLEAST.
10. Boolean formula can be nested. For example, "g1 := a & b | c ^ ~d | ~(e|f)"
"""
from __future__ import print_function

from collections import deque
import os
import re

import argparse as ap


class ParsingError(Exception):
    """General parsing errors."""
    pass


class FormatError(Exception):
    """Common errors in writing the shorthand format."""
    pass


class FaultTreeError(Exception):
    """Indication of problems in the fault tree."""
    pass


class Node(object):
    """Representation of a base class for a node in a fault tree.

    Attributes:
        name: A specific name that identifies this node.
    """

    def __init__(self, name):
        self.name = name
        self.__parents = set()

    def add_parent(self, formula):
        """Adds a formula as a parent of the node.

        Args:
            formula: The formula where this node appears.
        """
        assert formula not in self.__parents
        assert type(formula) is Formula
        self.__parents.add(formula)

    def is_orphan(self):
        """Determines if the node is parentless."""
        return len(self.__parents) == 0


class BasicEvent(Node):
    """Representation of a basic event in a fault tree.

    Attributes:
        prob: Probability of failure of this basic event.
    """

    def __init__(self, name, prob):
        super(BasicEvent, self).__init__(name)
        self.prob = prob


class HouseEvent(Node):
    """Representation of a house event in a fault tree.

    Attributes:
        state: State of the house event ("true" or "false").
    """

    def __init__(self, name, state):
        super(HouseEvent, self).__init__(name)
        self.state = state


class Gate(Node):
    """Representation of a gate of a fault tree.

    Attributes:
        mark: Marking for various algorithms like toposort.
        formula: The formula of this gate.
    """

    def __init__(self, name, formula):
        super(Gate, self).__init__(name)
        self.mark = None  # marking for various algorithms
        self.formula = formula  # the formula of this gate


class Formula(object):
    """Boolean formula with arguments.

    Attributes:
        operator: Logical operator of this formula.
        k_num: Min number for the combination operator.
        node_arguments: String names of non-formula arguments like basic events.
        f_arguments: arguments that are formulas.
        g_arguments: arguments that are gates.
        b_arguments: arguments that are basic events.
        h_arguments: arguments that are house events.
        u_arguments: arguments that are undefined from the input.
    """

    def __init__(self, operator=None, k_num=None):
        self.operator = operator
        self.k_num = k_num
        self.node_arguments = []
        self.f_arguments = []
        self.g_arguments = []
        self.b_arguments = []
        self.h_arguments = []
        self.u_arguments = []

    def num_arguments(self):
        """Returns the number of arguments."""
        return len(self.f_arguments) + len(self.node_arguments)


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
        if (self.basic_events.has_key(name.lower()) or
            self.gates.has_key(name.lower()) or
            self.house_events.has_key(name.lower())):
            raise FaultTreeError("Redefinition of a node: " + name)

    def __detect_cycle(self):
        """Checks if the fault tree has a cycle.

        Raises:
            FaultTreeError: There is a cycle in the fault tree.
        """
        def continue_formula(formula):
            """Continues visiting gates in the formula.

            This is a helper function of visit(gate) function.

            Args:
                formula: The formula to be visited further.

            Returns:
                None if no cycle is found.
                A list of node names in a detected cycle path in reverse order.
            """
            for gate in formula.g_arguments:
                cycle = visit(gate)
                if cycle:
                    return cycle
            for arg in formula.f_arguments:
                cycle = continue_formula(arg)
                if cycle:
                    return cycle
            return None

        def visit(gate):
            """Recursively visits the given gate sub-tree to detect a cycle.

            Upon visiting the descendant gates, their marks are changed from
            temporary to permanent.

            Args:
                gate: The current gate.

            Returns:
                None if no cycle is found.
                A list of node names in a detected cycle path in reverse order.
            """
            if not gate.mark:
                gate.mark = "temp"
                cycle = continue_formula(gate.formula)
                if cycle:
                    cycle.append(gate.name)
                    return cycle
                gate.mark = "perm"
            elif gate.mark == "temp":
                return [gate.name]  # a cycle is detected
            return None  # the permanent mark

        def print_cycle(cycle):
            """Prints the detected cycle.

            Args:
                cycle: A list of gate names in the cycle path in reverse order.

            Raises:
                FaultTreeError: There is a cycle in the fault tree.
            """
            start = cycle[0]
            cycle.reverse()
            raise FaultTreeError("Detected a cycle: " +
                                 str([x for x in cycle[cycle.index(start):]]))

        assert self.top_gates is not None
        for top_gate in self.top_gates:
            cycle = visit(top_gate)
            if cycle:
                print_cycle(cycle)

        detached_gates = [x for x in self.gates.itervalues() if not x.mark]
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
        top_gates = [x for x in self.gates.itervalues() if x.is_orphan()]
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

    def add_gate(self, name, formula):
        """Creates and adds a new gate into the fault tree.

        Args:
            name: A name for the new gate.
            gate_type: A gate type for the new gate.
            children: Collection of children names of the new gate.
            k_num: K number is required for a combination type of a gate.

        Raises:
            FaultTreeError: The given name already exists.
        """
        self.__check_redefinition(name)
        gate = Gate(name, formula)
        self.gates.update({name.lower(): gate})

    def populate(self):
        """Assigns children to gates and parents to children.

        Raises:
            FaultTreeError: There are problems with the fault tree.
        """
        def populate_formula(formula):
            """Assigns children to formula.

            Args:
                formula: A formula with arguments.
            """
            assert formula.num_arguments() > 0
            for child in formula.node_arguments:
                child_node = None
                if self.gates.has_key(child.lower()):
                    child_node = self.gates[child.lower()]
                    formula.g_arguments.append(child_node)
                elif self.basic_events.has_key(child.lower()):
                    child_node = self.basic_events[child.lower()]
                    formula.b_arguments.append(child_node)
                elif self.house_events.has_key(child.lower()):
                    child_node = self.house_events[child.lower()]
                    formula.h_arguments.append(child_node)
                elif self.undef_nodes.has_key(child.lower()):
                    child_node = self.undef_nodes[child.lower()]
                    formula.u_arguments.append(child_node)
                else:
                    print("Warning. Unidentified node: " + child)
                    child_node = Node(child)
                    self.undef_nodes.update({child.lower(): child_node})
                    formula.u_arguments.append(child_node)
                child_node.add_parent(formula)

            for child_formula in formula.f_arguments:
                populate_formula(child_formula)

        for gate in self.gates.itervalues():
            populate_formula(gate.formula)

        for basic in self.basic_events.itervalues():
            if basic.is_orphan():
                print("Warning. Orphan basic event: " + basic.name)
        for house in self.house_events.itervalues():
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
    # Fault tree name
    ft_name_re = re.compile(r"^\s*(" + name_sig + r")\s*$")
    # Node names
    name_re = re.compile(r"\s*(" + name_sig + r")\s*$")
    # General gate name and pattern
    gate_sig = r"^\s*(\w+)\s*:=\s*"
    gate_re = re.compile(gate_sig + r"(.+)$")
    # Optional parentheses for formulas
    paren_re = re.compile(r"\s*\((.+)\)\s*$")
    # Formula
    form = r"([^()]*\(.+\)[^()]*|[^()]+)"
    # AND gate identification
    and_re = re.compile(r"(\s*" + form + r"(\s*&\s*" + form + r"\s*)+)$")
    # OR gate identification
    or_re = re.compile(r"(\s*" + form + r"(\s*\|\s*" + form + r"\s*)+)$")
    # Combination gate identification
    comb_children = r"\[(\s*.+(\s*,\s*.+\s*){2,})\]"
    comb_re = re.compile(r"@\(([2-9])\s*,\s*" + comb_children + r"\s*\)\s*$")
    # NOT gate identification
    not_re = re.compile(r"~\s*(\w+|@?\(.+\))$")
    # XOR gate identification
    xor_re = re.compile(r"(\s*" + form + r"\s*\^\s*" + form + r"\s*)$")
    # Probability description for a basic event
    prob_re = re.compile(r"^\s*p\(\s*(\w+)\s*\)\s*=\s*(1|0|0\.\d+)\s*$")
    # State description for a house event
    state_re = re.compile(r"^\s*s\(\s*(\w+)\s*\)\s*=\s*(true|false)\s*$")

    blank_line = re.compile(r"^\s*$")

    ft_name = None
    fault_tree = FaultTree()

    def check_parentheses(line):
        """Verifies correct formatting for closing and opening parentheses.

        Args:
            line: A string containing a Boolean formula.

        Raises:
            FormatError: There are problems with parentheses.
        """
        # The simplest check
        if line.count("(") != line.count(")"):
            raise FormatError("Opening and closing parentheses do not match.")

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
        splitter = "\\" + splitter
        split_re = re.compile(r"(?!\([^)]*)" + splitter + r"(?![^(]*\))")
        arguments = split_re.split(arguments_string)
        arguments = [x.strip() for x in arguments]
        if len(arguments) > len(set([x.lower() for x in arguments])):
            raise FaultTreeError("Repeated arguments:\n" + arguments_string)
        # This is a hack for XOR with more than 2 arguments.
        if splitter == "\\^" and len(arguments) > 2:
            hacked_args = arguments[1:]
            joined_args = hacked_args[0]
            for i in range(1, len(hacked_args)):
                joined_args += "^" + hacked_args[i]
            arguments = [arguments[0], joined_args]

        return arguments

    def get_formula(line):
        """Constructs formula from the given line.

        Args:
            line: A string containing a Boolean equation.

        Returns:
            A Formula object.

        Raises:
            ParsingError: Parsing is unsuccessful.
            FormatError: Formatting problems in the input.
            FaultTreeError: Problems in the structure of the formula.
        """
        if paren_re.match(line):
            formula_line = paren_re.match(line).group(1)
            return get_formula(formula_line)
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
        elif comb_re.match(line):
            k_num, arguments = comb_re.match(line).group(1, 2)
            arguments = get_arguments(arguments, ",")
            if int(k_num) >= len(arguments):
                raise FaultTreeError(
                        "Invalid k/n for the combination formula:\n" + line)
            operator = "atleast"
        elif not_re.match(line):
            arguments = not_re.match(line).group(1)
            arguments = get_arguments(arguments, "~")
            operator = "not"
        else:
            raise ParsingError("Cannot interpret the formula:\n" + line)
        formula = Formula(operator, k_num)
        for arg in arguments:
            if name_re.match(arg):
                formula.node_arguments.append(arg)
            else:
                formula.f_arguments.append(get_formula(arg))
        return formula

    shorthand_file = open(input_file, "r")
    for line in shorthand_file:
        try:
            if blank_line.match(line):
                continue
            elif gate_re.match(line):
                gate_name, formula_line = gate_re.match(line).group(1, 2)
                check_parentheses(formula_line)
                fault_tree.add_gate(gate_name, get_formula(formula_line))
            elif prob_re.match(line):
                event_name, prob = prob_re.match(line).group(1, 2)
                fault_tree.add_basic(event_name, prob)
            elif state_re.match(line):
                event_name, state = state_re.match(line).group(1, 2)
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
            raise ParsingError(str(err) + "\nIn the following line:\n" + line)
        except FormatError as err:
            raise FormatError(str(err) + "\nIn the following line:\n" + line)
        except FaultTreeError as err:
            raise FaultTreeError(str(err) + "\nIn the following line:\n" + line)
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
    def continue_formula(formula, final_list):
        """Continues visiting gates in the formula.

        This is a helper function of visit(gate) function.

        Args:
            formula: The formula to be visited further.
            final_list: A deque of sorted gates.
        """
        for gate in formula.g_arguments:
            visit(gate, final_list)
        for arg in formula.f_arguments:
            continue_formula(arg, final_list)

    def visit(gate, final_list):
        """Recursively visits the given gate sub-tree to include into the list.

        Args:
            gate: The current gate.
            final_list: A deque of sorted gates.
        """
        assert gate.mark != "temp"
        if not gate.mark:
            gate.mark = "temp"
            continue_formula(gate.formula, final_list)
            gate.mark = "perm"
            final_list.appendleft(gate)

    sorted_gates = deque()
    for top_gate in top_gates:
        visit(top_gate, sorted_gates)
    assert len(sorted_gates) == len(gates)
    return sorted_gates

def write_to_xml_file(fault_tree, output_file):
    """Writes the fault tree into an XML file.

    The file is formatted according to OpenPSA MEF but without indentations
    for human readability.

    Args:
        fault_tree: A full fault tree.
        output_file: The output destination.
    """
    t_file = open(output_file, "w")
    t_file.write("<?xml version=\"1.0\"?>\n")
    t_file.write("<opsa-mef>\n")
    t_file.write("<define-fault-tree name=\"%s\">\n" % fault_tree.name)

    def write_formula(formula, o_file):
        """Write the formula in OpenPSA MEF XML.

        Args:
            formula: The formula to be printed.
            o_file: The output file stream.
        """
        o_file.write("<" + formula.operator)
        if formula.operator == "atleast":
            o_file.write(" min=\"" + formula.k_num + "\"")
        o_file.write(">\n")
        # Print house events
        for h_child in formula.h_arguments:
            o_file.write("<house-event name=\"" + h_child.name + "\"/>\n")

        # Print basic events
        for b_child in formula.b_arguments:
            o_file.write("<basic-event name=\"" + b_child.name + "\"/>\n")

        # Print undefined events
        for u_child in formula.u_arguments:
            o_file.write("<event name=\"" + u_child.name + "\"/>\n")

        # Print gates
        for g_child in formula.g_arguments:
            o_file.write("<gate name=\"" + g_child.name + "\"/>\n")

        # Print formulas
        for f_child in formula.f_arguments:
            write_formula(f_child, o_file)

        o_file.write("</" + formula.operator+ ">\n")

    def write_gate(gate, o_file):
        """Write the gate in OpenPSA MEF XML.

        Args:
            gate: The gate to be printed.
            o_file: The output file stream.
        """
        o_file.write("<define-gate name=\"" + gate.name + "\">\n")
        write_formula(gate.formula, o_file)
        o_file.write("</define-gate>\n")

    sorted_gates = toposort_gates(fault_tree.top_gates,
                                  fault_tree.gates.values())
    for gate in sorted_gates:
        write_gate(gate, t_file)

    t_file.write("</define-fault-tree>\n")

    if fault_tree.basic_events or fault_tree.house_events:
        t_file.write("<model-data>\n")
        for basic in fault_tree.basic_events.itervalues():
            t_file.write("<define-basic-event name=\"" + basic.name + "\">\n"
                        "<float value=\"" + str(basic.prob) + "\"/>\n"
                        "</define-basic-event>\n")

        for house in fault_tree.house_events.itervalues():
            t_file.write("<define-house-event name=\"" + house.name + "\">\n"
                        "<constant value=\"" + str(house.state) + "\"/>\n"
                        "</define-house-event>\n")
        t_file.write("</model-data>\n")

    t_file.write("</opsa-mef>")
    t_file.close()


def main():
    """Verifies arguments and calls parser and writer.

    Raises:
        ArgumentTypeError: There are problemns with the arguments.
    """
    description = "A script that converts shorthand fault tree notation into"\
                  " an OpenPSA MEF XML file."

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
