#!/usr/bin/env python
"""fault_tree_generator.py

A script to generate a fault tree of various complexities. The generated
fault tree is put into XML file with OpenPSA MEF ready for analysis.
This script should help create complex fault trees to test analysis tools.
"""
from __future__ import print_function, division

import Queue
import argparse as ap
import random
import sys
import time

sys.setrecursionlimit(int(1e6))  # heavy use of recursion for big trees


class Node(object):
    """Representation of a base class for a node in a fault tree.

    Attributes:
        name: A specific name that identifies this node.
        parents: A set of parents of this node.
    """
    def __init__(self, name="", parent=None):
        self.name = name
        self.parents = set()
        if parent:
            parent.add_child(self)

    def is_shared(self):
        """Indicates if this node appears in several places."""
        return len(self.parents) > 1

    def num_parents(self):
        """Returns the number of unique parents."""
        return len(self.parents)


class Gate(Node):
    """Representation of a gate of a fault tree.

        Names are assigned sequentially starting from G0.
        G0 is assumed to be the root gate.

    Attributes:
        num_gates: Total number of gates created.
        gate_types: Types of gates that are allowed in the fault tree.
        gates: A set of all gates that are created for the fault tree.
        b_children: Children of this gate that are basic events.
        h_children: Children of this gate that are house events.
        g_children: Children of this gate that are gates.
        gate_type: Type of the gate. Chosen randomly.
    """
    num_gates = 0  # to keep track of gates and to name them
    gate_types = ["and", "or", "atleast", "not", "xor"]
    gate_weights = [1, 1, 0, 0, 0]  # weights for the random choice
    gates = []  # container for all created gates

    def __init__(self, parent=None):
        super(Gate, self).__init__("G" + str(Gate.num_gates), parent)
        Gate.num_gates += 1  # post-decrement to account for the root gate
        self.b_children = set()  # children that are basic events
        self.h_children = set()  # children that are house events
        self.g_children = set()  # children that are gates
        self.k_num = None
        self.gate_type = self.get_random_type()
        Gate.gates.append(self)  # keep track of all gates

    def get_random_type(self):
        cum_dist = [x / sum(Gate.gate_weights) for x in Gate.gate_weights]
        cum_dist.insert(0, 0)
        for i in range(1, len(cum_dist)):
            cum_dist[i] += cum_dist[i - 1]
        r_num = random.random()
        bin_num = [i - 1 for i in range(1, len(cum_dist))
                    if cum_dist[i - 1] <= r_num < cum_dist[i]]
        assert len(bin_num) == 1
        return Gate.gate_types[bin_num[0]]

    def num_children(self):
        """Returns the number of children."""
        return (len(self.b_children) + len(self.h_children) +
                len(self.g_children))

    def add_child(self, child):
        """Adds child into a collection of gate or primary event children.

        Note that this function also updates the parent set of the child.

        Args:
            child: Gate, HouseEvent, or BasicEvent child.
        """
        child.parents.add(self)
        if type(child) is Gate:
            self.g_children.add(child)
        elif type(child) is BasicEvent:
            self.b_children.add(child)
        elif type(child) is HouseEvent:
            self.h_children.add(child)
        else:
            sys.exit("Illegal child type for a gate.")

    def has_ancestor(self, gate):
        """Finds if the given gate is an ancestor of the node.

        Args:
            gate: A potential ancestor gate.

        Returns:
            True or false.
        """
        if gate is self:
            return True

        if gate in self.parents:
            return True

        for parent in self.parents:
            if parent.has_ancestor(gate):
                return True

        return False


class BasicEvent(Node):
    """Representation of a basic event in a fault tree.

        Names are assigned sequentially starting from E1.

    Attributes:
        num_basic: Total number of basic events created.
        min_prob: Lower bound of the distribution.
        max_prob: Upper bound of the distribution.
        prob: Probability of failure of this basic event. Assigned randomly.
        basic_events: A set of all basic events created for the fault tree.
    """
    num_basic = 0
    min_prob = 0
    max_prob = 1
    basic_events = []  # container for created basic events
    non_ccf_events = []  # basic events that are not in ccf groups
    def __init__(self, parent=None):
        BasicEvent.num_basic += 1
        super(BasicEvent, self).__init__("E" + str(BasicEvent.num_basic),
                                           parent)
        self.prob = random.uniform(BasicEvent.min_prob, BasicEvent.max_prob)
        BasicEvent.basic_events.append(self)


class HouseEvent(Node):
    """Representation of a house event in a fault tree.

        Names are assigned sequentially starting from H1.

    Attributes:
        num_house: Total number of house events created.
        state: The state of the house event.
        house_events: A set of all house events created for the fault tree.
    """
    num_house = 0
    house_events = []  # container for created house events
    def __init__(self, parent=None):
        HouseEvent.num_house += 1
        super(HouseEvent, self).__init__("H" + str(HouseEvent.num_house),
                                         parent)
        self.state = random.choice(["true", "false"])
        HouseEvent.house_events.append(self)


class CcfGroup(object):
    num_ccf = 0
    ccf_groups = []
    def __init__(self):
        CcfGroup.num_ccf += 1
        self.name = "CCF" + str(CcfGroup.num_ccf)
        self.members = []
        self.prob = random.uniform(BasicEvent.min_prob, BasicEvent.max_prob)
        self.model = "MGL"
        CcfGroup.ccf_groups.append(self)

    def get_factors(self):
        assert len(self.members) > 1
        levels = random.randint(2, len(self.members))
        factors = [random.uniform(0.1, 1) for i in range(levels - 1)]
        return factors


def generate_fault_tree(args):
    """Generates a fault tree of specified complexity from command-line
    arguments.

    Args:
        args: Configurations for fault tree construction.

    Returns:
        Top gate of the created fault tree.
    """
    BasicEvent.min_prob = args.minprob
    BasicEvent.max_prob = args.maxprob

    min_children = 2  # minimum number of children per gate
    max_children = args.nchildren * 2 - min_children

    def init_gates(args, gates_queue):
        """Initialize intermediate gates and other primary events.

        Args:
            args: Configurations for the fault tree.
            gates_queue: Queue of gates to be initialized.
        """
        if gates_queue.empty():
            return

        # Get an intermediate gate to intialize breadth-first
        gate = gates_queue.get()

        # Sample children size
        num_children = random.randint(min_children, max_children)

        if gate.gate_type == "not":
            num_children = 1
        elif gate.gate_type == "xor":
            num_children = 2
        elif gate.gate_type == "atleast":
            num_children = num_children if num_children > 2 else 3
            gate.k_num = random.randint(2, num_children - 1)

        while gate.num_children() < num_children:
            # Case when the number of primary events is already satisfied
            if len(BasicEvent.basic_events) == args.nprimary:
                # Reuse already initialized primary events
                gate.add_child(random.choice(BasicEvent.basic_events))
                continue

            # Sample gates vs. primary events
            s_ratio = random.random()
            s_reuse = random.random()  # sample the reuse frequency
            if s_ratio < (1.0 / (1 + args.ratio)):
                # Create a new gate or reuse an existing one
                if s_reuse < args.reuse_g:
                    random.shuffle(Gate.gates)
                    for random_gate in Gate.gates:
                        if random_gate in gate.g_children:
                            continue
                        if (not random_gate.g_children or
                                not gate.has_ancestor(random_gate)):
                            gate.add_child(random_gate)
                            break
                else:
                    gates_queue.put(Gate(gate))
            else:
                # Create a new primary event or reuse an existing one
                if s_reuse < args.reuse_p and BasicEvent.basic_events:
                    # Reuse an already initialized primary event
                    gate.add_child(random.choice(BasicEvent.basic_events))
                else:
                    BasicEvent(gate)

        # Corner case when not enough new primary events initialized, but
        # there are no more intermediate gates to use due to a big ratio
        # or just random accident.
        if (gates_queue.empty() and
                len(BasicEvent.basic_events) < args.nprimary):
            # Initialize more gates by randomly choosing places in the
            # fault tree.
            random_gate = random.choice(Gate.gates)
            while (random_gate.gate_type == "not" or
                   random_gate.gate_type == "xor"):
                random_gate = random.choice(Gate.gates)
            gates_queue.put(Gate(random_gate))

        init_gates(args, gates_queue)

    # Start with a top event
    top_event = Gate()
    while top_event.gate_type != "and" and top_event.gate_type != "or":
        top_event.gate_type = top_event.get_random_type()
    top_event.name = args.root
    num_children = random.randint(min_children, max_children)

    # Configuring the number of children for the top event
    if args.ctop:
        num_children = args.ctop
    elif num_children < args.ptop:
        num_children = args.ptop

    # Container for not yet initialized gates
    # Queue is used to traverse the tree breadth-first
    gates_queue = Queue.Queue()

    # Initialize the top root node
    while len(top_event.b_children) < args.ptop:
        BasicEvent(top_event)
    while top_event.num_children() < num_children:
        gates_queue.put(Gate(top_event))

    # Procede with children gates
    init_gates(args, gates_queue)

    # Distribute house events
    while len(HouseEvent.house_events) < args.house:
        target_gate = random.choice(Gate.gates)
        if (target_gate is not top_event and
                target_gate.gate_type != "xor" and
                target_gate.gate_type != "not"):
            HouseEvent(target_gate)

    # Create CCF groups from the existing basic events.
    if args.ccf:
        members = BasicEvent.basic_events[:]
        random.shuffle(members)
        first_mem = 0
        last_mem = 0
        while len(CcfGroup.ccf_groups) < args.ccf:
            last_mem = first_mem + random.randint(2, 5)
            if last_mem > len(members):
                break
            CcfGroup().members = members[first_mem : last_mem]
            first_mem = last_mem
        BasicEvent.non_ccf_events = members[first_mem : ]

    return top_event


def write_info(args):
    """Writes the information about the setup and generated fault tree.

    This function uses the output destination from the arguments.

    Args:
        args: Command-line configurations.
    """
    t_file = open(args.out, "w")
    t_file.write("<?xml version=\"1.0\"?>\n")
    t_file.write(
            "<!--\nThis is an autogenerated fault tree description\n"
            "with the following parameters:\n\n"
            "The output file name: " + str(args.out) + "\n"
            "The fault tree name: " + args.ft_name + "\n"
            "The root gate name: " + args.root + "\n\n"
            "The seed of the random number generator: " + str(args.seed) + "\n"
            "The number of basic events: " + str(args.nprimary) + "\n"
            "The number of house events: " + str(args.house) + "\n"
            "The number of CCF groups: " + str(args.ccf) + "\n"
            "The average number of children per gate: " +
            str(args.nchildren) + "\n"
            "Basic events to gates ratio per new node: " +
            str(args.ratio) + "\n"
            "The weights of gate types [AND, OR, K/N, NOT, XOR]: " +
            str(Gate.gate_weights) + "\n"
            "Approximate percentage of repeated primary events in the tree: " +
            str(args.reuse_p) + "\n"
            "Approximate percentage of repeated gates in the tree: " +
            str(args.reuse_g) + "\n"
            "Maximum probability for basic events: " +
            str(args.maxprob) + "\n"
            "Minimum probability for basic events: " +
            str(args.minprob) + "\n"
            "Minimal number of primary events for the root node: " +
            str(args.ptop) + "\n"
            "-->\n"
            )

    shared_p = [x for x in BasicEvent.basic_events if x.is_shared()]
    shared_g = [x for x in Gate.gates if x.is_shared()]
    and_gates = [x for x in Gate.gates if x.gate_type == "and"]
    or_gates = [x for x in Gate.gates if x.gate_type == "or"]
    atleast_gates = [x for x in Gate.gates if x.gate_type == "atleast"]
    not_gates = [x for x in Gate.gates if x.gate_type == "not"]
    xor_gates = [x for x in Gate.gates if x.gate_type == "xor"]

    t_file.write(
            "<!--\nThe generated fault tree has the following metrics:\n\n"
            "The number of basic events: " +
            str(BasicEvent.num_basic) + "\n"
            "The number of house events: " + str(HouseEvent.num_house) + "\n"
            "The number of CCF groups: " + str(CcfGroup.num_ccf) + "\n"
            "The number of gates: " + str(Gate.num_gates) + "\n"
            "    AND gates: " + str(len(and_gates)) + "\n"
            "    OR gates: " + str(len(or_gates)) + "\n"
            "    K/N gates: " + str(len(atleast_gates)) + "\n"
            "    NOT gates: " + str(len(not_gates)) + "\n"
            "    XOR gates: " + str(len(xor_gates)) + "\n"
            "Basic events to gates ratio: " +
            str(BasicEvent.num_basic / Gate.num_gates) + "\n"
            "The average number of children per gate: " +
            str(sum(x.num_children() for x in Gate.gates) / len(Gate.gates)) +
            "\n"
            "The number of shared basic events: " + str(len(shared_p)) + "\n"
            "The number of shared gates: " + str(len(shared_g)) + "\n"
            )
    if shared_p:
        t_file.write(
                "The avg. number of parents of shared basic events: " +
                str(sum(x.num_parents() for x in shared_p) / len(shared_p)) +
                "\n"
                )
    if shared_g:
        t_file.write(
                "The avg. number of parents of shared gates: " +
                str(sum(x.num_parents() for x in shared_g) / len(shared_g)) +
                "\n"
                )

    t_file.write("-->\n\n")

def write_model_data(t_file, basic_events):
    """Appends model data with primary event descriptions.

    Args:
        t_file: The output stream.
        basic_events: A set of basic events.
    """
    # Print probabilities of basic events
    t_file.write("<model-data>\n")

    for basic in basic_events:
        t_file.write("<define-basic-event name=\"" + basic.name + "\">\n"
                     "<float value=\"" + str(basic.prob) + "\"/>\n"
                     "</define-basic-event>\n")

    for house in HouseEvent.house_events:
        t_file.write("<define-house-event name=\"" + house.name + "\">\n"
                     "<constant value=\"" + house.state + "\"/>\n"
                     "</define-house-event>\n")


    t_file.write("</model-data>\n")

def write_results(args, top_event):
    """Writes results of a generated fault tree.

    Writes the information about the fault tree in an XML file.
    The fault tree is printed breadth-first.
    The output XML file is not formatted for human readability.

    Args:
        args: Configurations of this fault tree generation process.
        top_event: Top gate of the generated fault tree.
    """
    # Plane text is used instead of any XML tools for performance reasons.
    write_info(args)
    t_file = open(args.out, "a")

    t_file.write("<opsa-mef>\n")
    t_file.write("<define-fault-tree name=\"%s\">\n" % args.ft_name)

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
            o_file.write(" min=\"" + str(gate.k_num) +"\"")
        o_file.write(">\n")
        # Print children that are gates.
        for g_child in gate.g_children:
            o_file.write("<gate name=\"" + g_child.name + "\"/>\n")
            # Update the queue
            gates_queue.put(g_child)

        # Print children that are basic events.
        for b_child in gate.b_children:
            o_file.write("<basic-event name=\"" + b_child.name + "\"/>\n")

        # Print children that are house events.
        for h_child in gate.h_children:
            o_file.write("<house-event name=\"" + h_child.name + "\"/>\n")

        o_file.write("</" + gate.gate_type+ ">\n")
        o_file.write("</define-gate>\n")

    # Write top event and update queue of intermediate gates
    write_gate(top_event, t_file)

    written_gates = set()

    # Proceed with intermediate gates
    while not gates_queue.empty():
        gate = gates_queue.get()
        if gate not in written_gates:
            written_gates.add(gate)
            write_gate(gate, t_file)

    # Proceed with ccf groups
    for ccf_group in CcfGroup.ccf_groups:
        t_file.write("<define-CCF-group name=\"" + ccf_group.name + "\""
                     " model=\"" + ccf_group.model + "\">\n"
                     "<members>\n")
        for member in ccf_group.members:
            t_file.write("<basic-event name=\"" + member.name + "\"/>\n")
        t_file.write("</members>\n<distribution>\n<float value=\"" +
                     str(ccf_group.prob) + "\"/>\n</distribution>\n")
        t_file.write("<factors>\n")
        factors = ccf_group.get_factors()
        level = 2
        for factor in factors:
            t_file.write("<factor level=\"" + str(level) + "\">\n"
                         "<float value=\"" + str(factor) + "\"/>\n</factor>\n")
            level += 1

        t_file.write("</factors>\n")
        t_file.write("</define-CCF-group>\n")

    t_file.write("</define-fault-tree>\n")

    if args.ccf:
        write_model_data(t_file, BasicEvent.non_ccf_events)
    else:
        write_model_data(t_file, BasicEvent.basic_events)

    t_file.write("</opsa-mef>")
    t_file.close()

def write_shorthand(args, top_event):
    t_file = open(args.out, "w")
    t_file.write(args.ft_name + "\n\n")
    # Container for not yet initialized intermediate events
    gates_queue = Queue.Queue()

    def write_gate(gate, o_file):
        """Print children for the gate.

        Note that it also updates the queue of gates.

        Args:
            gate: The gate to be printed.
            o_file: The output file stream.
        """
        line = [gate.name]
        line.append(" := ")
        div = ""
        line_end = ""
        if gate.gate_type == "and":
            line.append("(")
            line_end = ")"
            div = " & "
        elif gate.gate_type == "or":
            line.append("(")
            line_end = ")"
            div = " | "
        elif gate.gate_type == "atleast":
            line.append("@(" + str(gate.k_num) + ", [")
            line_end = "])"
            div = ", "

        first_child = True
        # Print children that are gates.
        for g_child in gate.g_children:
            if first_child:
                line.append(g_child.name)
                first_child = False
            else:
                line.append(div + g_child.name)
            # Update the queue
            gates_queue.put(g_child)

        # Print children that are basic events.
        for b_child in gate.b_children:
            if first_child:
                line.append(b_child.name)
                first_child = False
            else:
                line.append(div + b_child.name)

        line.append(line_end)
        o_file.write("".join(line))
        o_file.write("\n")

    # Write top event and update queue of intermediate gates
    write_gate(top_event, t_file)

    written_gates = set()

    # Proceed with intermediate gates
    while not gates_queue.empty():
        gate = gates_queue.get()
        if gate not in written_gates:
            written_gates.add(gate)
            write_gate(gate, t_file)

    # Write basic events
    t_file.write("\n")
    for basic in BasicEvent.basic_events:
        t_file.write("p(" + basic.name + ") = " + str(basic.prob) + "\n")


def check_if_positive(desc, val):
    """Verifies that the value is potive or zero for the supplied argument.

    Args:
        desc: The description of the argument from the command-line.
        val: The value of the argument.

    Raises:
        ArgumentTypeError: The value is negative.
    """
    if val < 0:
        raise ap.ArgumentTypeError(desc + " is negative")

def check_if_less(desc, val, ref):
    """Verifies that the value is less than some reference for
    the supplied argument.

    Args:
        desc: The description of the argument from the command-line.
        val: The value of the argument.
        ref: The reference value.

    Raises:
        ArgumentTypeError: The value is more than the reference.
    """
    if val > ref:
        raise ap.ArgumentTypeError(desc + " is more than " + str(ref))

def check_valid_setup(args):
    """Checks if the relationships between arguments are valid for use.

    These checks are important to ensure that the requested fault tree is
    producible and realistic to achieve in reasonable time.

    Raises:
        ArgumentTypeError: There are problemns with the arguments.
    """
    if args.maxprob < args.minprob:
        raise ap.ArgumentTypeError("Max probability < Min probability")

    if args.ptop > args.nprimary:
        raise ap.ArgumentTypeError("ptop > # of total primary events")

    if args.ptop > args.ctop:
        raise ap.ArgumentTypeError("ptop > # of children for top")

    if args.ptop and args.ptop == args.ctop and args.nprimary > args.ptop:
        raise ap.ArgumentTypeError("(ctop > ptop) is required to expand "
                                   "the tree")

    if args.house >= args.nprimary or args.nprimary - args.house <= args.ptop:
        raise ap.ArgumentTypeError("Too many house events")

    if args.ccf > args.nprimary / 2:
        raise ap.ArgumentTypeError("Too many ccf groups")

    if args.weights_g:
        if [i for i in args.weights_g if float(i) < 0]:
            raise ap.ArgumentTypeError("weights cannot be negative")

        if len(args.weights_g) > len(Gate.gate_weights):
            raise ap.ArgumentTypeError("too many weights are provided")

        weights_float = [float(i) for i in args.weights_g]
        for i in range(len(Gate.gate_weights) - len(weights_float)):
            weights_float.append(0)
        Gate.gate_weights = weights_float

    if args.shorthand:
        if args.out == "fault_tree.xml":
            args.out = "fault_tree.txt"
        if args.weights_g and len(args.weights_g) > 3:
            raise ap.ArgumentTypeError("No complex gate type representation "
                                       "for the shorthand format")
        if args.house:
            raise ap.ArgumentTypeError("No house event representation "
                                       "for the shorthand format")

def manage_cmd_args():
    """Manages command-line description and arguments.

    Returns:
        Arguments that are collected from the command line.

    Raises:
        ArgumentTypeError: There are problemns with the arguments.
    """
    description = "A script to create a fault tree of an arbitrary size and"\
                  " complexity."

    parser = ap.ArgumentParser(description=description)

    ft_name = "the name for the fault tree"
    parser.add_argument("--ft-name", type=str, help=ft_name,
                        default="Autogenerated")

    root = "the name for the root gate"
    parser.add_argument("--root", type=str, help=root, default="root")

    seed = "the seed of a random number generator"
    parser.add_argument("--seed", type=int, help=seed, default=123)

    nprimary = "the number of primary events"
    parser.add_argument("-p", "--nprimary", type=int, help=nprimary,
                        default=10)

    nchildren = "the average number of children per gate"
    parser.add_argument("-c", "--nchildren", type=int, help=nchildren,
                        default=3)

    ratio = "basic events to gates ratio per a new gate"
    parser.add_argument("--ratio", type=float, help=ratio, default=2)

    gate_weights = "weights for samling [AND, OR, K/N, NOT, XOR] gate types"
    parser.add_argument("--weights-g", type=str, nargs="*", help=gate_weights)

    reuse_p = "approximate percentage of repeated primary events in the tree"
    parser.add_argument("--reuse-p", type=float, help=reuse_p, default=0.1)

    reuse_g = "approximate percentage of repeated gates in the tree"
    parser.add_argument("--reuse-g", type=float, help=reuse_g, default=0.1)

    maxprob = "maximum probability for primary events"
    parser.add_argument("--maxprob", type=float, help=maxprob,
                        default=0.1)

    minprob = "minimum probability for primary events"
    parser.add_argument("--minprob", type=float, help=minprob,
                        default=0.001)

    ptop = "minimal number of primary events for a root node"
    parser.add_argument("--ptop", type=int, help=ptop,
                        default=0)

    ctop = "minimal number of children for a root node"
    parser.add_argument("--ctop", type=int, help=ctop,
                        default=0)  # 0 indicates that the number is not set.

    house = "the number of house events"
    parser.add_argument("--house", type=int, help=house, default=0)

    ccf = "the number of ccf groups"
    parser.add_argument("--ccf", type=int, help=ccf, default=0)

    out = "output file to write the generated fault tree"
    parser.add_argument("-o", "--out", help=out, default="fault_tree.xml")

    shorthand = "shorthand format for the output"
    parser.add_argument("--shorthand", action="store_true", help=shorthand)

    args = parser.parse_args()

    # Check for validity of arguments
    check_if_positive(ctop, args.ctop)
    check_if_positive(ptop, args.ptop)
    check_if_positive(ratio, args.ratio)
    check_if_positive(nchildren, args.nchildren)
    check_if_positive(nprimary, args.nprimary)
    check_if_positive(minprob, args.minprob)
    check_if_positive(maxprob, args.maxprob)
    check_if_positive(reuse_p, args.reuse_p)
    check_if_positive(reuse_g, args.reuse_g)
    check_if_positive(house, args.house)
    check_if_positive(ccf, args.ccf)

    check_if_less(reuse_p, args.reuse_p, 0.9)
    check_if_less(reuse_g, args.reuse_p, 0.9)
    check_if_less(maxprob, args.maxprob, 1)
    check_if_less(minprob, args.minprob, 1)

    check_valid_setup(args)
    return args


def main():
    """The main fuction of the fault tree generator.

    Raises:
        ArgumentTypeError: There are problemns with the arguments.
    """
    args = manage_cmd_args()
    # Set the seed for this tree generator
    random.seed(args.seed)

    top_event = generate_fault_tree(args)

    # Write output files
    if args.shorthand:
        write_shorthand(args, top_event)
    else:
        write_results(args, top_event)

if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as error:
        print("Argument Error:")
        print(error)
