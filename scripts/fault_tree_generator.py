#!/usr/bin/env python
"""fault_tree_generator.py

A script to generate a fault tree of various complexities. The generated
fault tree is put into XML file with OpenPSA MEF ready for analysis.
This script should help create complex fault trees to test analysis tools.
"""
from __future__ import print_function

import Queue
import random

import argparse as ap


class Node(object):
    """Representation of a base class for a node in a fault tree.

    Attributes:
        name: A specific name that identifies this node.
        parents: A set of parents of this node.
    """
    def __init__(self, name=""):
        self.name = name
        self.parents = set()

    def is_shared(self):
        """Indicates if this node appears in several places."""
        return self.parents > 1

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
        p_children: Children of this gate that are primary events.
        g_children: Children of this gate that are gates.
        gate_type: Type of the gate. Chosen randomly.
        ancestors: Ancestor gates of this gate.
    """
    num_gates = 0  # to keep track of gates and to name them
    gate_types = ["or", "and"]  # supported types of gates
    gates = set()  # container for all created gates

    def __init__(self):
        super(Gate, self).__init__("G" + str(Gate.num_gates))
        Gate.num_gates += 1  # post-decrement to account for the root gate
        self.p_children = set()  # children that are primary events
        self.g_children = set()  # children that are gates
        self.gate_type = random.choice(Gate.gate_types)  # type of a gate
        self.ancestors = set()
        self.ancestors.add(self)
        Gate.gates.add(self)  # keep track of all gates

    def num_children(self):
        """Returns the number of children."""
        return len(self.p_children) + len(self.g_children)


class PrimaryEvent(Node):
    """Representation of a primary event in a fault tree.

        Names are assigned sequentially starting from E1.

    Attributes:
        num_primary: Total number of primary events created.
        min_prob: Lower bound of the distribution.
        max_prob: Upper bound of the distribution.
        prob: Probability of failure of this primary event. Assigned randomly.
        primary_events: A set of all primary events created for the fault tree.
    """
    num_primary = 0
    min_prob = 0
    max_prob = 1
    primary_events = []  # container for created primary events
    def __init__(self):
        PrimaryEvent.num_primary += 1
        super(PrimaryEvent, self).__init__("E" + str(PrimaryEvent.num_primary))
        self.prob = random.uniform(PrimaryEvent.min_prob,
                                   PrimaryEvent.max_prob)
        PrimaryEvent.primary_events.append(self)


def create_gate(parent):
    """Handles proper creation of gates.

    The new gate is created with type, parent, and ancestor information.
    The type is chosen randomly.

    Args:
        parent: The parent gate for the new gate.

    Returns:
        A newly created gate.
    """
    gate = Gate()
    parent.g_children.add(gate)
    gate.parents.add(parent)
    gate.ancestors.update(parent.ancestors)
    return gate

def create_primary(parent):
    """Handles proper creation of primary events

    The new primary event is given a random probability. The parent and
    child information is updated.

    Args:
        parent: The parent gate of the new primary event.

    Returns:
        A newly created primary event.
    """
    primary_event = PrimaryEvent()
    parent.p_children.add(primary_event)
    primary_event.parents.add(parent)
    return primary_event

def generate_fault_tree(args):
    """Generates a fault tree of specified complexity from command-line
    arguments.

    Args:
        args: Configurations for fault tree construction.

    Returns:
        Top gate of the created fault tree.
    """
    PrimaryEvent.min_prob = args.minprob
    PrimaryEvent.max_prob = args.maxprob

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

        while gate.num_children() < num_children:
            # Case when the number of primary events is already satisfied
            if len(PrimaryEvent.primary_events) == args.nprimary:
                # Reuse already initialized primary events
                gate.p_children.add(random.choice(PrimaryEvent.primary_events))
                continue

            # Sample inter events vs. primary events
            s_ratio = random.random()
            if s_ratio < (1.0 / (1 + args.ratio)):
                # Create a new gate or reuse an existing one
                gates_queue.put(create_gate(gate))
            else:
                # Create a new primary event or reuse an existing one
                s_reuse = random.random()
                if s_reuse < args.reuse_p and PrimaryEvent.primary_events:
                    # Reuse an already initialized primary event
                    gate.p_children.add(
                            random.choice(PrimaryEvent.primary_events))
                else:
                    create_primary(gate)

        # Corner case when not enough new primary events initialized, but
        # there are no more intemediate gates to use due to the low ratio
        # or just random accident.
        if (gates_queue.empty() and
                len(PrimaryEvent.primary_events) < args.nprimary):
            # Initialize more gates by randomly choosing places in the
            # fault tree. The number of new gates depends on the required
            # number of new primary events.
            gates_queue.put(create_gate(gate))

        init_gates(args, gates_queue)

    # Start with a top event
    top_event = Gate()
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
    while len(top_event.p_children) < args.ptop:
        create_primary(top_event)
    while top_event.num_children() < num_children:
        gates_queue.put(create_gate(top_event))

    # Procede with children gates
    init_gates(args, gates_queue)

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
            "The seed of a random number generator: " + str(args.seed) + "\n"
            "The number of unique primary events: " + str(args.nprimary) + "\n"
            "The average number of children per gate: " +
            str(args.nchildren) + "\n"
            "Primary events to gates ratio per new node: " +
            str(args.ratio) + "\n"
            "Approximate percentage of repeated primary events in the tree: " +
            str(args.reuse_p) + "\n"
            "Approximate percentage of repeated gates in the tree: " +
            str(args.reuse_g) + "\n"
            "Maximum probability for primary events: " +
            str(args.maxprob) + "\n"
            "Minimum probability for primary events: " +
            str(args.minprob) + "\n"
            "Minimal number of primary events for the root node: " +
            str(args.ptop) + "\n"
            "-->\n"
            )
    t_file.write(
            "<!--\nThe generated fault tree has the following metrics:\n\n"
            "The number of primary events: " +
            str(PrimaryEvent.num_primary) + "\n"
            "The number of gates: " + str(Gate.num_gates) + "\n"
            "Primary events to gates ratio: " +
            str(PrimaryEvent.num_primary * 1.0 / Gate.num_gates) + "\n"
            "-->\n\n"
            )

def write_model_data(t_file, primary_events):
    """Appends model data with primary event descriptions.

    Args:
        t_file: The output stream.
        primary_events: A set of primary events.
    """
    # Print probabilities of primary events
    t_file.write("<model-data>\n")
    for primary in primary_events:
        t_file.write("<define-basic-event name=\"" + primary.name + "\">\n"
                     "<float value=\"" + str(primary.prob) + "\"/>\n"
                     "</define-basic-event>\n")

    t_file.write("</model-data>\n")

def write_results(args, top_event, primary_events):
    """Writes results of a generated fault tree.

    Writes the information about the fault tree in an XML file.
    The fault tree is printed breadth-first.
    The output XML file is not formatted for human readability.

    Args:
        args: Configurations of this fault tree generation process.
        top_event: Top gate of the generated fault tree.
        primary_events: A set of primary events of the fault tree.
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
        o_file.write("<" + gate.gate_type + ">\n")
        # Print primary events
        for p_child in gate.p_children:
            o_file.write("<basic-event name=\"" + p_child.name + "\"/>\n")

        # Print intermediate events
        for g_child in gate.g_children:
            o_file.write("<gate name=\"" + g_child.name + "\"/>\n")
            # Update the queue
            gates_queue.put(g_child)

        o_file.write("</" + gate.gate_type+ ">\n")
        o_file.write("</define-gate>\n")

    # Write top event and update queue of intermediate gates
    write_gate(top_event, t_file)

    # Proceed with intermediate gates
    while not gates_queue.empty():
        gate = gates_queue.get()
        write_gate(gate, t_file)

    t_file.write("</define-fault-tree>\n")

    write_model_data(t_file, primary_events)

    t_file.write("</opsa-mef>")
    t_file.close()


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


def main():
    """Verifies arguments and calls fault tree generator functions.

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

    nprimary = "the number of unique primary events"
    parser.add_argument("-p", "--nprimary", type=int, help=nprimary,
                        default=10)

    nchildren = "the average number of children per gate"
    parser.add_argument("-c", "--nchildren", type=int, help=nchildren,
                        default=3)

    ratio = "primary events to gates ratio per a new gate"
    parser.add_argument("--ratio", type=float, help=ratio, default=2)

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

    out = "output file to write the generated fault tree"
    parser.add_argument("-o", "--out", help=out, default="fault_tree.xml")

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

    check_if_less(reuse_p, args.reuse_p, 0.9)
    check_if_less(reuse_g, args.reuse_p, 0.9)
    check_if_less(maxprob, args.maxprob, 1)
    check_if_less(minprob, args.minprob, 1)

    if args.maxprob < args.minprob:
        raise ap.ArgumentTypeError("Max probability < Min probability")

    if args.ptop > args.nprimary:
        raise ap.ArgumentTypeError("ptop > # of total primary events")

    if args.ptop > args.ctop:
        raise ap.ArgumentTypeError("ptop > # of children for top")

    if args.ptop and args.ptop == args.ctop and args.nprimary > args.ptop:
        raise ap.ArgumentTypeError("(ctop > ptop) is required to expand "
                                   "the tree")

    # Set the seed for this tree generator
    random.seed(args.seed)

    top_event = generate_fault_tree(args)

    # Write output files
    write_results(args, top_event, PrimaryEvent.primary_events)


if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as error:
        print("Argument Error:")
        print(error)
