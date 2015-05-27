#!/usr/bin/env python
"""shorthand_to_xml.py

This script converts the shorthand notation for fault trees into an
XML file. The output file is formatted according to OpenPSA MEF.

The shorthand notation is described as follows:
AND gates:                       gate_name := (child1 & child2 & ...)
OR gates:                        gate_name := (child1 | child2 | ...)
ATLEAST(k/n) gates:              gate_name := @(k, [child1, child2, ...])
NOT gates:                       gate_name := ~child
XOR gates:                       gate_name := (child1 ^ child2)
Probabilities of basic events:   p(event_name) = probability

Some requirements to the shorthand input file:
0. The names are not case-sensitive.
1. The fault tree name must be formatted according to 'XML NCNAME datatype'.
2. No requirement for the structure of the input, i.e. topologically sorted.
3. Undefined nodes are processed as 'events' to the final XML output. However,
    warnings will be emitted in case it is the user's mistake.
4. Name clashes or redefinitions are errors.
5. Cyclic trees are detected by the script.
6. The top gate is detected by the script. Only one top gate is allowed.
7. Repeated children are considered an error.
8. The script is flexibile with white spaces in the input file.
"""
from __future__ import print_function

from collections import deque
import os
import re
import sys

import argparse as ap


class Node(object):
    """Representation of a base class for a node in a fault tree.

    Attributes:
        name: A specific name that identifies this node.
        parents: A set of parents of this node.
    """
    def __init__(self, name=None):
        self.name = name
        self.parents = set()

class BasicEvent(Node):
    """Representation of a basic event in a fault tree.

    Attributes:
        prob: Probability of failure of this basic event.
    """
    def __init__(self, name=None, prob=None):
        super(BasicEvent, self).__init__(name)
        self.prob = prob

class Gate(Node):
    """Representation of a gate of a fault tree.

    Attributes:
        children: A list of children names.
        g_children: Children of this gate that are gates.
        p_children: Children of this gate that are basic events.
        u_children: Children that are undefined from the input.
        gate_type: Type of the gate. Chosen randomly.
        k_num: Min number for a combination gate.
        mark: Marking for various algorithms like toposort.
    """
    def __init__(self, name=None, gate_type=None, k_num=None):
        super(Gate, self).__init__(name)
        self.gate_type = gate_type
        self.k_num = k_num
        self.children = []
        self.g_children = []
        self.p_children = []
        self.u_children = []  # undefined children
        self.mark = None  # marking for various algorithms

class FaultTree(object):
    """Representation of a fault tree for shorthand to XML purposes.

    Attributes:
        name: The name of a fault tree.
        gates: A collection of gates of a fault tree.
        basic_events: A collection of basic events of a fault tree.
        undef_nodes: Nodes that are not explicitly defined as gates or
            basic events.
        top_gate: The top gate of a fault tree.
    """
    def __init__(self, name=None):
        self.name = name
        self.gates = {}
        self.basic_events = {}
        self.undef_nodes = {}
        self.top_gate = None

    def __check_redefinition(self, name):
        """Checks if a node is being redefined.

        Args:
            name: The name under investigation.
        """
        if (self.basic_events.has_key(name.lower()) or
            self.gates.has_key(name.lower())):
            sys.exit("Redefinition of a node: " + name)

    def __detect_cycle(self):
        """Checks if the fault tree has a cycle."""
        def visit(gate):
            """Recursively visits the given gate sub-tree to detect a cycle.

            Upon visiting the children gates, their marks are changed from
            temporary to permanent.

            Args:
                gate: The current gate.

            Returns:
                None if no cycle is found.
                A list of node names in a detected cycle path in reverse order.
            """
            if not gate.mark:
                gate.mark = "temp"
                for child in gate.g_children:
                    cycle = visit(child)
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
            """
            start = cycle[0]
            cycle.reverse()
            sys.exit("Detected a cycle: " +
                     str([x for x in cycle[cycle.index(start):]]))

        assert self.top_gate is not None
        cycle = visit(self.top_gate)
        if cycle:
            print_cycle(cycle)

        detached_gates = [x for x in self.gates.itervalues() if not x.mark]
        if detached_gates:
            error_msg = "Detected detached gates that may be in a cycle\n"
            error_msg += str([x.name for x in detached_gates])
            print(error_msg)
            for gate in detached_gates:
                cycle = visit(gate)
                if cycle:
                    print_cycle(cycle)
            sys.exit()

    def __detect_top(self):
        """Detects the top gate of the developed fault tree."""
        top_gates = [x for x in self.gates.itervalues() if not x.parents]
        if len(top_gates) > 1:
            names = [x.name for x in top_gates]
            sys.exit("Detected multiple top gates:\n" + str(names))
        elif not top_gates:
            sys.exit("No top gate is detected")
        self.top_gate = top_gates[0]

    def add_basic(self, name, prob):
        """Creates and adds a new basic event into the fault tree.

        Args:
            name: A name for the new basic event.
            prob: The probability of the new basic event.
        """
        self.__check_redefinition(name)
        self.basic_events.update({name.lower(): BasicEvent(name, prob)})

    def add_gate(self, name, gate_type, children, k_num=None):
        """Creates and adds a new gate into the fault tree.

        Args:
            name: A name for the new gate.
            gate_type: A gate type for the new gate.
            children: Collection of children names of the new gate.
            k_num: K number is required for a combination type of a gate.
        """
        self.__check_redefinition(name)
        gate = Gate(name, gate_type, k_num)
        gate.children = children
        self.gates.update({name.lower(): gate})

    def populate(self):
        """Assigns children to gates and parents to children."""
        for gate in self.gates.itervalues():
            for child in gate.children:
                child_node = None
                if self.gates.has_key(child.lower()):
                    child_node = self.gates[child.lower()]
                    gate.g_children.append(child_node)
                elif self.basic_events.has_key(child.lower()):
                    child_node = self.basic_events[child.lower()]
                    gate.p_children.append(child_node)
                elif self.undef_nodes.has_key(child.lower()):
                    child_node = self.undef_nodes[child.lower()]
                    gate.u_children.append(child_node)
                else:
                    print("Warning. Unidentified node: " + child)
                    child_node = Node(child)
                    self.undef_nodes.update({child.lower(): child_node})
                    gate.u_children.append(child_node)
                child_node.parents.add(gate)
        self.__detect_top()
        self.__detect_cycle()


def parse_input_file(input_file):
    """Parses an input file with a shorthand description of a fault tree.

    Args:
        input_file: The path to the input file.

    Returns:
        The fault tree described in the input file.
    """
    short_file = open(input_file, "r")
    # Fault tree name
    ft_name_re = re.compile(r"^\s*(\w+)\s*$")
    gate_sig = r"^\s*(\w+)\s*:=\s*"
    gate_re = re.compile(gate_sig + r".*")
    # AND gate identification
    and_re = re.compile(gate_sig + r"\((\s*\w+(\s*&\s*\w+\s*)+)\)\s*$")
    # OR gate identification
    or_re = re.compile(gate_sig + r"\((\s*\w+(\s*\|\s*\w+\s*)+)\)\s*$")
    # Combination gate identification
    comb_children = r"\[(\s*\w+(\s*,\s*\w+\s*){2,})\]"
    comb_re = re.compile(gate_sig + r"@\(([2-9])\s*,\s*" + comb_children +
                         r"\s*\)\s*$")
    # NOT gate identification
    not_re = re.compile(gate_sig + r"~\s*(\w+)")
    # XOR gate identification
    xor_re = re.compile(gate_sig + r"\((\s*\w+\s*\^\s*\w+\s*)\)")
    # Probability description for a basic event
    prob_re = re.compile(r"^\s*p\(\s*(\w+)\s*\)\s*=\s*(0\.\d+)\s*$")

    blank_line = re.compile(r"^\s*$")

    ft_name = None
    fault_tree = FaultTree()

    def get_gate_children(children_string, splitter, line):
        """Splits the input string into children of a gate.

        If a repeated child is found, halts the script with a message.

        Args:
            children_string: String contaning children names.
            splitter: Splitter specific to the parent gate, i.e. "&", "|", ','.
            line: The line containing the string. It is needed for error
                messages.

        Returns:
            Children list from the input string.
        """
        children = children_string.split(splitter)
        children = [x.strip() for x in children]
        if len(children) > len(set([x.lower() for x in children])):
            sys.exit("Repeated children:\n" + line)
        return children

    for line in short_file:
        if blank_line.match(line):
            continue
        elif gate_re.match(line):
            if and_re.match(line):
                gate_name, children = and_re.match(line).group(1, 2)
                children = get_gate_children(children, "&", line)
                fault_tree.add_gate(gate_name, "and", children)
            elif or_re.match(line):
                gate_name, children = or_re.match(line).group(1, 2)
                children = get_gate_children(children, "|", line)
                fault_tree.add_gate(gate_name, "or", children)
            elif comb_re.match(line):
                gate_name, k_num, children = comb_re.match(line).group(1, 2, 3)
                children = get_gate_children(children, ",", line)
                if int(k_num) >= len(children):
                    sys.exit("Invalid k/n for a combination gate:\n" + line)
                fault_tree.add_gate(gate_name, "atleast", children, k_num)
            elif not_re.match(line):
                gate_name, children = not_re.match(line).group(1, 2)
                children = get_gate_children(children, "~", line)
                fault_tree.add_gate(gate_name, "not", children)
            elif xor_re.match(line):
                gate_name, children = xor_re.match(line).group(1, 2)
                children = get_gate_children(children, "^", line)
                fault_tree.add_gate(gate_name, "xor", children)
        elif prob_re.match(line):
            event_name, prob = prob_re.match(line).group(1, 2)
            fault_tree.add_basic(event_name, prob)
        elif ft_name_re.match(line):
            if ft_name:
                sys.exit("Redefinition of the fault tree name:\n" + line)
            ft_name = ft_name_re.match(line).group(1)
        else:
            sys.exit("Cannot interpret the following line:\n" + line)

    if ft_name is None:
        sys.exit("The fault tree name is not given.")
    fault_tree.name = ft_name
    fault_tree.populate()
    return fault_tree


def toposort_gates(top_gate, gates):
    """Sorts gates topologically starting from the root gate.

    Args:
        top_gate: The root gate of the fault tree.
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
            for child in gate.g_children:
                visit(child, final_list)
            gate.mark = "perm"
            final_list.appendleft(gate)
    sorted_gates = deque()
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

    def write_gate(gate, o_file):
        """Print children for the gate.

        Args:
            gate: The gate to be printed.
            o_file: The output file stream.
        """

        o_file.write("<define-gate name=\"" + gate.name + "\">\n")
        o_file.write("<" + gate.gate_type)
        if gate.gate_type == "atleast":
            o_file.write(" min=\"" + gate.k_num + "\"")
        o_file.write(">\n")
        # Print intermediate gates
        for g_child in gate.g_children:
            o_file.write("<gate name=\"" + g_child.name + "\"/>\n")

        # Print basic events
        for p_child in gate.p_children:
            o_file.write("<basic-event name=\"" + p_child.name + "\"/>\n")

        # Print undefined events
        for u_child in gate.u_children:
            o_file.write("<event name=\"" + u_child.name + "\"/>\n")

        o_file.write("</" + gate.gate_type+ ">\n")
        o_file.write("</define-gate>\n")

    sorted_gates = toposort_gates(fault_tree.top_gate,
                                  fault_tree.gates.values())

    for gate in sorted_gates:
        write_gate(gate, t_file)

    t_file.write("</define-fault-tree>\n")

    t_file.write("<model-data>\n")
    for basic in fault_tree.basic_events.itervalues():
        t_file.write("<define-basic-event name=\"" + basic.name + "\">\n"
                     "<float value=\"" + str(basic.prob) + "\"/>\n"
                     "</define-basic-event>\n")
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

    out = "output file to write the converted input"
    parser.add_argument("-o", "--out", help=out)

    args = parser.parse_args()

    if not args.input_file:
        raise ap.ArgumentTypeError("No input file is provided.")

    fault_tree = parse_input_file(args.input_file)
    out = args.out
    if not out:
        out = os.path.basename(args.input_file)
        out = out[:out.rfind(".")] + ".xml"
    write_to_xml_file(fault_tree, out)

if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as error:
        print("Argument Error:")
        print(error)
