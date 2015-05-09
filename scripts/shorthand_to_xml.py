#!/usr/bin/env python
"""shorthand_to_xml.py

This script converts the shorthand notation for fault trees into an
XML file. The output file is formatted according to OpenPSA MEF.
"""
from __future__ import print_function

import os
import re
import sys

import Queue

import argparse as ap


class Node(object):
    def __init__(self, name=None):
        self.name = name
        self.parents = set()

class PrimaryEvent(Node):
    def __init__(self, name=None, prob=None):
        super(PrimaryEvent, self).__init__(name)
        self.prob = prob

class Gate(Node):
    def __init__(self, name=None, gate_type=None, k_num=None):
        super(Gate, self).__init__(name)
        self.gate_type = gate_type
        self.k_num = k_num
        self.children = []
        self.g_children = []
        self.p_children = []
        self.u_children = []  # undefined children

class FaultTree(object):
    def __init__(self, name=None):
        self.name = name
        self.gates = {}
        self.primary_events = {}
        self.undef_nodes = {}
        self.top_gate = None

    def __check_redefinition(self, name):
        if (self.primary_events.has_key(name.lower()) or
            self.gates.has_key(name.lower())):
            sys.exit("Redefinition of a node: " + name)

    def __check_cyclicity(self):
        visited = set()
        def check(gate, path):
            if gate in path:
                path.append(gate)  # for printing
                sys.exit("Detected a cycle: " +
                         str([x.name for x in path[path.index(gate):]]))
            if gate in visited:
                return
            visited.add(gate)
            path.append(gate)
            for child in gate.g_children:
                check(child, path[:])

        for gate in self.gates.itervalues():
            check(gate, [])

    def __detect_top(self):
        top_gates = [x for x in self.gates.itervalues() if not x.parents]
        if len(top_gates) > 1:
            names = [x.name for x in top_gates]
            sys.exit("Detected multiple top gates:\n" + str(names))
        self.top_gate = top_gates[0]

    def add_primary(self, name, prob):
        self.__check_redefinition(name)
        self.primary_events.update({name.lower(): PrimaryEvent(name, prob)})

    def add_gate(self, name, gate_type, children=None, k_num=None):
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
                elif self.primary_events.has_key(child.lower()):
                    child_node = self.primary_events[child.lower()]
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
        self.__check_cyclicity()
        self.__detect_top()


def parse_input_file(input_file):
    short_file = open(input_file, "r")
    # Fault tree name
    ft_name_re = re.compile(r"^\s*(\w+)\s*$")
    gate_sig = r"^\s*(\w+)\s*:=\s*"
    # AND gate identification
    and_re = re.compile(gate_sig + r"\((\s*\w+(\s*&\s*\w+\s*)+)\)\s*$")
    # OR gate identification
    or_re = re.compile(gate_sig + r"\((\s*\w+(\s*\|\s*\w+\s*)+)\)\s*$")
    # Combination gate identification
    comb_children = r"\[(\s*\w+(\s*,\s*\w+\s*){2,})\]"
    comb_re = re.compile(gate_sig + r"@\(([2-9])\s*,\s*" + comb_children +
                         r"\s*\)\s*$")
    # Probability description for a basic event
    prob_re = re.compile(r"^\s*p\(\s*(\w+)\s*\)\s*=\s*(0\.\d+)\s*$")

    blank_line = re.compile(r"^\s*$")

    ft_name = None
    fault_tree = FaultTree()

    def get_gate_children(children_string, splitter, line):
        """Splits the input string into children of a gate.

        If a repeated child is found, halts the script with a message.

        Returns:
            Children from the string.
        """
        children = children_string.split(splitter)
        children = [x.strip() for x in children]
        if len(children) > len(set([x.lower() for x in children])):
            sys.exit("Repeated children:\n" + line)
        return children

    for line in short_file:
        if blank_line.match(line):
            continue
        elif ft_name_re.match(line):
            if ft_name:
                sys.exit("Redefinition of the fault tree name:\n" + line)
            ft_name = ft_name_re.match(line).group(1)
        elif and_re.match(line):
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
        elif prob_re.match(line):
            event_name, prob = prob_re.match(line).group(1, 2)
            fault_tree.add_primary(event_name, prob)
        else:
            sys.exit("Cannot interpret the following line:\n" + line)

    if ft_name is None:
        sys.exit("The fault tree name is not given.")
    fault_tree.name = ft_name
    fault_tree.populate()
    return fault_tree


def write_to_xml_file(fault_tree, output_file):
    """Writes the fault tree into an XML file.
    """
    t_file = open(output_file, "w")
    t_file.write("<?xml version=\"1.0\"?>\n")
    t_file.write("<opsa-mef>\n")
    t_file.write("<define-fault-tree name=\"%s\">\n" % fault_tree.name)
    # Container for not yet initialized intermediate events
    gates_queue = Queue.Queue()

    def write_gate(gate, o_file):
        """Print children for the gate.

        Note that it also updates the queue of gates.

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
            # Update the queue
            gates_queue.put(g_child)

        # Print primary events
        for p_child in gate.p_children:
            o_file.write("<basic-event name=\"" + p_child.name + "\"/>\n")

        # Print undefined events
        for u_child in gate.u_children:
            o_file.write("<event name=\"" + u_child.name + "\"/>\n")

        o_file.write("</" + gate.gate_type+ ">\n")
        o_file.write("</define-gate>\n")

    # Write top event and update queue of intermediate gates
    write_gate(fault_tree.top_gate, t_file)

    written_gates = set()

    # Proceed with intermediate gates
    while not gates_queue.empty():
        gate = gates_queue.get()
        if gate not in written_gates:
            written_gates.add(gate)
            write_gate(gate, t_file)

    t_file.write("</define-fault-tree>\n")

    t_file.write("<model-data>\n")
    for primary in fault_tree.primary_events.itervalues():
        t_file.write("<define-basic-event name=\"" + primary.name + "\">\n"
                     "<float value=\"" + str(primary.prob) + "\"/>\n"
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
