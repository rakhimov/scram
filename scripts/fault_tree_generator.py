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

"""Generates a fault tree of various complexities.

The generated fault tree can be put into an XML file with the OpenPSA MEF
or a shorthand format file.
The resulting fault tree is topologically sorted.

This script helps create complex fault trees in a short time
to test other analysis tools,
for example, input dependent performance analysis.

The time complexity is approximately:

    O(N) + O((N/Ratio)^2*exp(-AvgChildren/Ratio)) + O(CommonG*exp(CommonB))

Where N is the number of basic events,
and Ratio is N / num_gates.

Note that generating a fault tree
with both the number of basic events and the number of gates contstrained
may change other factors that are set by the user.
However, if the number of gates are not set (constrained) by the user,
all the other factors set by the user are
guaranteed to be preserved and used as they are.
"""

from __future__ import print_function, division

from collections import deque
import random

import argparse as ap


class Node(object):
    """Representation of a base class for a node in a fault tree.

    Attributes:
        name: A specific name that identifies this node.
        parents: A set of parents of this node.
    """

    def __init__(self, name, parent=None):
        """Constructs a node and adds it as a child for the parent.

        Note that the tracking of parents introduces a cyclic reference.

        Args:
            name: Identifier for the node.
            parent: Optional initial parent of this node.
        """
        self.name = name
        self.parents = set()
        if parent:
            parent.add_child(self)

    def is_common(self):
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
        gates: A set of all gates that are created for the fault tree.
        b_children: Children of this gate that are basic events.
        h_children: Children of this gate that are house events.
        g_children: Children of this gate that are gates.
        gate_type: Type of the gate. Chosen randomly.
        k_num: K number for a K/N combination gate. Chosen randomly.
        mark: Marking for various algorithms.
    """

    num_gates = 0  # to keep track of gates and to name them
    gates = []  # container for all created gates

    def __init__(self, parent=None):
        """Initializes a new gate.

        The unique identifier is created with the static variables.

        Args:
            parent: Optional initial parent of this node.
        """
        super(Gate, self).__init__("G" + str(Gate.num_gates), parent)
        Gate.num_gates += 1  # post-decrement to account for the root gate
        self.b_children = set()  # children that are basic events
        self.h_children = set()  # children that are house events
        self.g_children = set()  # children that are gates
        self.k_num = None
        self.gate_type = Factors.get_random_type()
        Gate.gates.append(self)  # keep track of all gates
        self.mark = None

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
        if isinstance(child, Gate):
            self.g_children.add(child)
        elif isinstance(child, BasicEvent):
            self.b_children.add(child)
        else:
            assert isinstance(child, HouseEvent)
            self.h_children.add(child)

    def get_ancestors(self):
        """Collects ancestors from this gate.

        Returns:
            A set of ancestors.
        """
        ancestors = set([self])
        parents = deque(self.parents)
        while parents:
            parent = parents.popleft()
            if parent not in ancestors:
                ancestors.add(parent)
                parents.extend(parent.parents)
        return ancestors


class BasicEvent(Node):
    """Representation of a basic event in a fault tree.

        Names are assigned sequentially starting from B1.

    Attributes:
        num_basic: Total number of basic events created.
        min_prob: Lower bound of the distribution.
        max_prob: Upper bound of the distribution.
        prob: Probability of failure of this basic event. Assigned randomly.
        basic_events: A list of all basic events created for the fault tree.
        non_ccf_events: A list of basic events that not in CCF groups.
    """

    num_basic = 0
    min_prob = 0
    max_prob = 1
    basic_events = []  # container for created basic events
    non_ccf_events = []  # basic events that are not in ccf groups

    def __init__(self, parent=None):
        """Initializes a basic event with a unique identifier.

        Args:
            parent: Optional initial parent of this node.
        """
        BasicEvent.num_basic += 1
        super(BasicEvent, self).__init__("B" + str(BasicEvent.num_basic),
                                         parent)
        self.prob = random.uniform(BasicEvent.min_prob, BasicEvent.max_prob)
        BasicEvent.basic_events.append(self)


class HouseEvent(Node):
    """Representation of a house event in a fault tree.

    Names are assigned sequentially starting from H1.

    Attributes:
        num_house: Total number of house events created.
        state: The state of the house event.
        house_events: A list of all house events created for the fault tree.
    """

    num_house = 0
    house_events = []  # container for created house events

    def __init__(self, parent=None):
        """Initializes a house event with a unique identifier.

        Args:
            parent: Optional initial parent of this node.
        """
        HouseEvent.num_house += 1
        super(HouseEvent, self).__init__("H" + str(HouseEvent.num_house),
                                         parent)
        self.state = random.choice(["true", "false"])
        HouseEvent.house_events.append(self)


class CcfGroup(object):
    """Representation of CCF groups in a fault tree.

    Names are assigned sequentially starting from CCF1.

    Attributes:
        num_ccf: The total number of CCF groups.
        ccf_groups: A collection of created CCF groups.
        name: The name of an instance CCF group.
        prob: Probability for a CCF group.
        model: The CCF model chosen for a group.
        members: A collection of members in a CCF group.
    """

    num_ccf = 0
    ccf_groups = []

    def __init__(self):
        """Constructs a unique CCF group with factors."""
        CcfGroup.num_ccf += 1
        self.name = "CCF" + str(CcfGroup.num_ccf)
        self.members = []
        self.prob = random.uniform(BasicEvent.min_prob, BasicEvent.max_prob)
        self.model = "MGL"
        CcfGroup.ccf_groups.append(self)

    def get_factors(self):
        """Generates CCF factors.

        Returns:
            A list of CCF factors.
        """
        assert len(self.members) > 1
        levels = random.randint(2, len(self.members))
        factors = [random.uniform(0.1, 1) for _ in range(levels - 1)]
        return factors


class Factors(object):
    """Collection of factors that determine the complexity of the fault tree.

    This collection must be setup and updated
    before the fault tree generation processes.

    Attributes:
        num_basics: The number of basic events.
        num_house: The number of house events.
        num_ccf: The number of ccf groups.
        common_b: The percentage of common basic events per gate.
        common_g: The percentage of common gates per gate.
        avg_children: The average number of children for gates.
        parents_b: The average number of parents for common basic events.
        parents_g: The average number of parents for common gates.
    """

    # Factors from the arguments
    num_basics = None
    num_house = None
    num_ccf = None
    common_b = None
    common_g = None
    avg_children = None
    parents_b = None
    parents_g = None
    __weights_g = None  # should not be set directly

    # Constant configurations
    __gate_types = ["and", "or", "atleast", "not", "xor"]

    # Calculated factors
    __norm_weights = []  # normalized weights
    __cum_dist = []  # CDF from the weights of the gate types
    __max_children = None  # the upper bound for the number of children
    __ratio = None  # basic events to gates ratio per gate
    __percent_basics = None  # percentage of basic events in gate children
    __percent_gates = None  # percentage of gates in gate children

    # Special case with the constrained number of gates
    __num_gates = None  # If set, all other factors get affected.

    @staticmethod
    def __calculate_max_children(avg_children, weights):
        """Calculates the maximum number of children for sampling.

        The result may have a fractional part
        that must be adjusted in sampling accordingly.

        Args:
            avg_children: The average number of children for gates.
            weights: Normalized weights for gate types.

        Returns:
            The upper bound for sampling in symmetric distributions.
        """
        # Min numbers for AND, OR, K/N, NOT, XOR types.
        min_children = [2, 2, 3, 1, 2]
        # Note that max and min numbers are the same for NOT and XOR.
        const_children = min_children[3:]
        const_weights = weights[3:]
        const_contrib = [x * y for x, y in zip(const_children, const_weights)]
        # AND, OR, K/N gate types can have the varying number of children.
        var_children = min_children[:3]
        var_weights = weights[:3]
        var_contrib = [x * y for x, y in zip(var_children, var_weights)]

        # AND, OR, K/N gate types can have the varying number of children.
        # Since the distribution is symmetric, the average is (max + min) / 2.
        return ((2 * avg_children - sum(var_contrib) - 2 * sum(const_contrib)) /
                sum(var_weights))

    @staticmethod
    def calculate():
        """Calculates any derived factors from the setup.

        This function must be called after all public factors are initialized.
        """
        Factors.__max_children = Factors.__calculate_max_children(
            Factors.avg_children,
            Factors.__norm_weights)
        g_factor = 1 - Factors.common_g + Factors.common_g / Factors.parents_g
        Factors.__ratio = Factors.avg_children * g_factor - 1
        Factors.__percent_basics = Factors.__ratio / (1 + Factors.__ratio)
        Factors.__percent_gates = 1 / (1 + Factors.__ratio)

    @staticmethod
    def get_weights():
        """Provides weights for gate types.

        Returns:
            Expected to return weights from the arguments.
        """
        assert Factors.__weights_g is not None
        return Factors.__weights_g

    @staticmethod
    def set_weights(weights):
        """Updates gate type weights.

        Args:
            weights: Weights of gate types.
        """
        assert len(weights) == len(Factors.__gate_types)
        assert sum(weights) > 0
        Factors.__weights_g = weights[:]
        Factors.__norm_weights = [x / sum(Factors.__weights_g)
                                  for x in Factors.__weights_g]
        Factors.__cum_dist = Factors.__norm_weights[:]
        Factors.__cum_dist.insert(0, 0)
        for i in range(1, len(Factors.__cum_dist)):
            Factors.__cum_dist[i] += Factors.__cum_dist[i - 1]

    @staticmethod
    def get_random_type():
        """Samples the gate type.

        Returns:
            A randomly chosen gate type.
        """
        r_num = random.random()
        bin_num = 1
        while Factors.__cum_dist[bin_num] <= r_num:
            bin_num += 1
        return Factors.__gate_types[bin_num - 1]

    @staticmethod
    def get_num_children(gate):
        """Randomly selects the number of children for the given gate type.

        This function has a side effect.
        It sets k_num for the K/N type of gates
        depending on the number of children.

        Args:
            gate: The parent gate for children.

        Returns:
            Random number of children.
        """
        if gate.gate_type == "not":
            return 1
        elif gate.gate_type == "xor":
            return 2

        max_children = int(Factors.__max_children)
        # Dealing with the fractional part.
        if random.random() < (Factors.__max_children - max_children):
            max_children += 1

        if gate.gate_type == "atleast":
            if max_children < 3:
                max_children = 3
            num_children = random.randint(3, max_children)
            gate.k_num = random.randint(2, num_children - 1)
            return num_children

        return random.randint(2, max_children)

    @staticmethod
    def get_percent_gates():
        """Returns the percentage of gates that should be in children."""
        return Factors.__percent_gates

    @staticmethod
    def get_num_gates():
        """Approximates the number of gates in the resulting fault tree.

        This is an estimate of the number of gates
        needed to initialize the fault tree
        with the given number of basic events
        and fault tree properties.

        Returns:
            The number of gates needed for the given basic events.
        """
        # Special case of constrained gates
        if Factors.__num_gates:
            return Factors.__num_gates
        b_factor = 1 - Factors.common_b + Factors.common_b / Factors.parents_b
        return int(Factors.num_basics /
                   (Factors.__percent_basics * Factors.avg_children * b_factor))

    @staticmethod
    def get_num_common_basics(num_gates):
        """Estimates the number of common basic events.

        These common basic events must be chosen
        from the total number of basic events
        in order to ensure the correct average number of parents.

        Args:
            num_gates: The total number of gates in the future fault tree

        Returns:
            The estimated number of common basic events.
        """
        return int(Factors.common_b * Factors.__percent_basics *
                   Factors.avg_children * num_gates / Factors.parents_b)

    @staticmethod
    def get_num_common_gates(num_gates):
        """Estimates the number of common gates.

        These common gates must be chosen
        from the total number of gates
        in order to ensure the correct average number of parents.

        Args:
            num_gates: The total number of gates in the future fault tree

        Returns:
            The estimated number of common gates.
        """
        return int(Factors.common_g * Factors.__percent_gates *
                   Factors.avg_children * num_gates / Factors.parents_g)

    @staticmethod
    def calculate_parents_g(num_gates):
        """Estimates the average number of parents for gates.

        This function makes possible to generate a fault tree
        with the user specified number of gates.
        All other parameters
        except for the number of parents
        must be set for use in calculations.

        Args:
            num_gates: The total number of gates in the future fault tree

        Returns:
            The estimated average number of parents of gates.
        """
        b_factor = 1 - Factors.common_b + Factors.common_b / Factors.parents_b
        ratio = 1 / (num_gates / Factors.num_basics *
                     Factors.avg_children * b_factor - 1)
        assert ratio > 0
        parents = Factors.common_g / (Factors.common_g - 1 +
                                      (1 + ratio) / Factors.avg_children)
        if parents < 2:
            parents = 2
        return parents

    @staticmethod
    def constrain_num_gates(num_gates):
        """Constrains the number of gates.

        The number of parents and the ratios for common nodes are manipulated.

        Args:
            num_gates: The total number of gates in the future fault tree
        """
        Factors.__num_gates = num_gates
        assert Factors.__num_gates * Factors.avg_children > Factors.num_basics
        # Calculate the ratios
        alpha = Factors.__num_gates / Factors.num_basics
        common = max(Factors.common_g, Factors.common_b)
        min_common = 1 - (1 + alpha) / Factors.avg_children / alpha
        if common < min_common:
            common = round(min_common + 0.05, 1)
        elif common > 2 * min_common:  # Really hope it does not happen
            common = 2 * min_common
        assert common < 1  # Very brittle configuration here

        Factors.common_g = common
        Factors.common_b = common
        parents = 1 / (1 - min_common / common)
        assert parents > 2  # This is brittle as well
        Factors.parents_g = parents
        Factors.parents_b = parents


class Settings(object):
    """Collection of settings specific to this script per run.

    These settings include arguments
    that do not influence the complexity
    of the generated fault tree.

    Attributes:
        ft_name: The name of the fault tree.
        root_name: The name of the root gate of the fault tree.
        seed: The seed for the pseudo-random number generator.
        output: The output destination.
        nested: A flag for the nested Boolean formula.
    """

    ft_name = None
    root_name = None
    seed = None
    output = None
    nested = None


def init_gates(gates_queue, common_basics, common_gates):
    """Initializes gates and other basic events.

    Args:
        gates_queue: A deque of gates to be initialized.
        common_basics: A list of common basic events.
        common_gates: A list of common gates.
    """
    # Get an intermediate gate to initialize breadth-first
    gate = gates_queue.popleft()

    num_children = Factors.get_num_children(gate)

    ancestors = None  # needed for cycle prevention
    max_tries = len(common_gates)  # the number of maximum tries
    num_trials = 0  # the number of trials to get common gate

    def candidate_gates():
        """Lazy generator of candidates for common gates.

        Yields:
            A next gate candidate from common gates container.
        """
        orphans = [x for x in common_gates if not x.parents]
        random.shuffle(orphans)
        for i in orphans:
            yield i

        single_parent = [x for x in common_gates if len(x.parents) == 1]
        random.shuffle(single_parent)
        for i in single_parent:
            yield i

        multi_parent = [x for x in common_gates if len(x.parents) > 1]
        random.shuffle(multi_parent)
        for i in multi_parent:
            yield i

    while gate.num_children() < num_children:
        s_percent = random.random()  # sample percentage of gates
        s_common = random.random()  # sample the reuse frequency

        # Case when the number of basic events is already satisfied
        if len(BasicEvent.basic_events) == Factors.num_basics:
            if not Factors.common_g and not Factors.common_b:
                # Reuse already initialized basic events
                gate.add_child(random.choice(BasicEvent.basic_events))
                continue
            else:
                s_common = 0  # use only common nodes

        if s_percent < Factors.get_percent_gates():
            # Create a new gate or use a common one
            if s_common < Factors.common_g and num_trials < max_tries:
                # Lazy evaluation of ancestors
                if not ancestors:
                    ancestors = gate.get_ancestors()

                for random_gate in candidate_gates():
                    num_trials += 1
                    if num_trials >= max_tries:
                        break
                    if random_gate in gate.g_children or random_gate is gate:
                        continue
                    if (not random_gate.g_children or
                            random_gate not in ancestors):
                        if not random_gate.parents:
                            gates_queue.append(random_gate)
                        gate.add_child(random_gate)
                        break
            else:
                gates_queue.append(Gate(gate))
        else:
            # Create a new basic event or use a common one
            if s_common < Factors.common_b and common_basics:
                orphans = [x for x in common_basics if not x.parents]
                if orphans:
                    gate.add_child(random.choice(orphans))
                    continue

                single_parent = [x for x in common_basics
                                 if len(x.parents) == 1]
                if single_parent:
                    gate.add_child(random.choice(single_parent))
                else:
                    gate.add_child(random.choice(common_basics))
            else:
                BasicEvent(gate)  # create a new basic event

    # Corner case when not enough new basic events initialized, but
    # there are no more intermediate gates to use due to a big ratio
    # or just random accident.
    if not gates_queue and len(BasicEvent.basic_events) < Factors.num_basics:
        # Initialize more gates by randomly choosing places in the
        # fault tree.
        random_gate = random.choice(Gate.gates)
        while (random_gate.gate_type == "not" or
               random_gate.gate_type == "xor" or
               random_gate in common_gates):
            random_gate = random.choice(Gate.gates)
        gates_queue.append(Gate(random_gate))


def generate_fault_tree():
    """Generates a fault tree of specified complexity.

    The Factors class attributes are used as parameters for complexity.

    Returns:
        Top gate of the created fault tree.
    """
    # Start with a top event
    top_event = Gate()
    while top_event.gate_type == "xor" or top_event.gate_type == "not":
        top_event.gate_type = Factors.get_random_type()
    top_event.name = Settings.root_name

    # Estimating the parameters
    num_gates = Factors.get_num_gates()
    num_common_basics = Factors.get_num_common_basics(num_gates)
    num_common_gates = Factors.get_num_common_gates(num_gates)
    common_basics = [BasicEvent() for _ in range(num_common_basics)]
    common_gates = [Gate() for _ in range(num_common_gates)]

    # Container for not yet initialized gates
    # A deque is used to traverse the tree breadth-first
    gates_queue = deque()
    gates_queue.append(top_event)

    # Proceed with children gates
    while gates_queue:
        init_gates(gates_queue, common_basics, common_gates)

    assert(not [x for x in BasicEvent.basic_events if not x.parents])
    assert(not [x for x in Gate.gates if not x.parents and x is not top_event])

    # Distribute house events
    while len(HouseEvent.house_events) < Factors.num_house:
        target_gate = random.choice(Gate.gates)
        if (target_gate is not top_event and
                target_gate.gate_type != "xor" and
                target_gate.gate_type != "not"):
            HouseEvent(target_gate)

    # Create CCF groups from the existing basic events.
    if Factors.num_ccf:
        members = BasicEvent.basic_events[:]
        random.shuffle(members)
        first_mem = 0
        last_mem = 0
        while len(CcfGroup.ccf_groups) < Factors.num_ccf:
            group_size = random.randint(2, int(2 * Factors.avg_children - 2))
            last_mem = first_mem + group_size
            if last_mem > len(members):
                break
            CcfGroup().members = members[first_mem:last_mem]
            first_mem = last_mem
        BasicEvent.non_ccf_events = members[first_mem:]

    return top_event


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


def write_info():
    """Writes the information about the setup and generated fault tree.

    This function uses the output destination from the arguments.
    """
    t_file = open(Settings.output, "w")
    t_file.write("<?xml version=\"1.0\"?>\n")
    t_file.write(
        "<!--\nThis is a description of the auto-generated fault tree\n"
        "with the following parameters:\n\n"
        "The output file name: " + Settings.output + "\n"
        "The fault tree name: " + Settings.ft_name + "\n"
        "The root gate name: " + Settings.root_name + "\n\n"
        "The seed of the random number generator: " +
        str(Settings.seed) + "\n"
        "The number of basic events: " + str(Factors.num_basics) + "\n"
        "The number of house events: " + str(Factors.num_house) + "\n"
        "The number of CCF groups: " + str(Factors.num_ccf) + "\n"
        "The average number of children for gates: " +
        str(Factors.avg_children) + "\n"
        "The weights of gate types [AND, OR, K/N, NOT, XOR]: " +
        str(Factors.get_weights()) + "\n"
        "Percentage of common basic events per gate: " +
        str(Factors.common_b) + "\n"
        "Percentage of common gates per gate: " +
        str(Factors.common_g) + "\n"
        "The avg. number of parents for common basic events: " +
        str(Factors.parents_b) + "\n"
        "The avg. number of parents for common gates: " +
        str(Factors.parents_g) + "\n"
        "Maximum probability for basic events: " +
        str(BasicEvent.max_prob) + "\n"
        "Minimum probability for basic events: " +
        str(BasicEvent.min_prob) + "\n"
        "-->\n")

    shared_b = [x for x in BasicEvent.basic_events if x.is_common()]
    shared_g = [x for x in Gate.gates if x.is_common()]
    and_gates = [x for x in Gate.gates if x.gate_type == "and"]
    or_gates = [x for x in Gate.gates if x.gate_type == "or"]
    atleast_gates = [x for x in Gate.gates if x.gate_type == "atleast"]
    not_gates = [x for x in Gate.gates if x.gate_type == "not"]
    xor_gates = [x for x in Gate.gates if x.gate_type == "xor"]

    frac_b = 0  # fraction of basic events in children per gate
    common_b = 0  # fraction of common basic events in basic events per gate
    common_g = 0  # fraction of common gates in gates per gate
    for gate in Gate.gates:
        num_b_children = len(gate.b_children)
        num_g_children = len(gate.g_children)
        frac_b += num_b_children / (num_g_children + num_b_children)
        if gate.b_children:
            num_common_b = len([x for x in gate.b_children if x.is_common()])
            common_b += num_common_b / num_b_children
        if gate.g_children:
            num_common_g = len([x for x in gate.g_children if x.is_common()])
            common_g += num_common_g / num_g_children
    num_gates = len(Gate.gates)
    common_b /= len([x for x in Gate.gates if x.b_children])
    common_g /= len([x for x in Gate.gates if x.g_children])
    frac_b /= num_gates

    t_file.write(
        "<!--\nThe generated fault tree has the following metrics:\n\n"
        "The number of basic events: %d" % BasicEvent.num_basic + "\n"
        "The number of house events: %d" % HouseEvent.num_house + "\n"
        "The number of CCF groups: %d" % CcfGroup.num_ccf + "\n"
        "The number of gates: %d" % Gate.num_gates + "\n"
        "    AND gates: %d" % len(and_gates) + "\n"
        "    OR gates: %d" % len(or_gates) + "\n"
        "    K/N gates: %d" % len(atleast_gates) + "\n"
        "    NOT gates: %d" % len(not_gates) + "\n"
        "    XOR gates: %d" % len(xor_gates) + "\n"
        "Basic events to gates ratio: %f" %
        (BasicEvent.num_basic / Gate.num_gates) + "\n"
        "The average number of children for gates: %f" %
        (sum(x.num_children() for x in Gate.gates) / len(Gate.gates)) + "\n"
        "The number of common basic events: %d" % len(shared_b) + "\n"
        "The number of common gates: %d" % len(shared_g) + "\n"
        "Percentage of common basic events per gate: %f" % common_b + "\n"
        "Percentage of common gates per gate: %f" % common_g + "\n"
        "Percentage of children that are basic events per gate: %f" %
        frac_b + "\n")
    if shared_b:
        t_file.write(
            "The avg. number of parents for common basic events: %f" %
            (sum(x.num_parents() for x in shared_b) / len(shared_b)) + "\n")
    if shared_g:
        t_file.write(
            "The avg. number of parents for common gates: %f" %
            (sum(x.num_parents() for x in shared_g) / len(shared_g)) + "\n")

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


def write_results(top_event):
    """Writes results of a generated fault tree into an XML file.

    Writes the information about the fault tree in an XML file.
    The fault tree is printed breadth-first.
    The output XML file is not formatted for human readability.

    Args:
        top_event: Top gate of the generated fault tree.
    """
    # Plane text is used instead of any XML tools for performance reasons.
    write_info()
    t_file = open(Settings.output, "a")

    t_file.write("<opsa-mef>\n")
    t_file.write("<define-fault-tree name=\"%s\">\n" % Settings.ft_name)

    nested_gates = set()  # collection of gates that got nested

    def write_gate(gate, o_file):
        """Print children for the gate.

        Args:
            gate: The gate to be printed.
            o_file: The output file stream.
        """
        def write_formula(gate, o_file):
            """Prints the formula of a gate.

            Args:
                gate: The gate to be printed.
                o_file: The output file stream.
            """
            o_file.write("<" + gate.gate_type)
            if gate.gate_type == "atleast":
                o_file.write(" min=\"" + str(gate.k_num) + "\"")
            o_file.write(">\n")
            # Print children that are house events.
            for h_child in gate.h_children:
                o_file.write("<house-event name=\"" + h_child.name + "\"/>\n")

            # Print children that are basic events.
            for b_child in gate.b_children:
                o_file.write("<basic-event name=\"" + b_child.name + "\"/>\n")

            # Print children that are gates.
            if not Settings.nested:
                for g_child in gate.g_children:
                    o_file.write("<gate name=\"" + g_child.name + "\"/>\n")
            else:
                to_nest = [x for x in gate.g_children if not x.is_common()]
                not_nest = [x for x in gate.g_children if x.is_common()]
                for g_child in not_nest:
                    o_file.write("<gate name=\"" + g_child.name + "\"/>\n")
                for g_child in to_nest:
                    write_formula(g_child, o_file)
                nested_gates.update(to_nest)

            o_file.write("</" + gate.gate_type + ">\n")

        o_file.write("<define-gate name=\"" + gate.name + "\">\n")
        write_formula(gate, o_file)
        o_file.write("</define-gate>\n")

    sorted_gates = toposort_gates(top_event, Gate.gates)
    for gate in sorted_gates:
        if Settings.nested and gate in nested_gates:
            continue
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

    if Factors.num_ccf:
        write_model_data(t_file, BasicEvent.non_ccf_events)
    else:
        write_model_data(t_file, BasicEvent.basic_events)

    t_file.write("</opsa-mef>")
    t_file.close()


def write_shorthand(top_event):
    """Writes the results into the shorthand format file.

    Note that the shorthand format does not support advanced gates or groups.

    Args:
        top_event: The top gate of the generated fault tree.
    """
    t_file = open(Settings.output, "w")
    t_file.write(Settings.ft_name + "\n\n")

    nested_gates = set()

    def write_gate(gate, o_file):
        """Print children for the gate.

        Args:
            gate: The gate to be printed.
            o_file: The output file stream.
        """
        def write_formula(gate, line):
            """Prints the formula of a gate in the shorthand format.

            Args:
                gate: The gate to be printed.
                line: The array containing strings to be joined for printing.
            """
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
            elif gate.gate_type == "not":
                line.append("~")
                line_end = ""
                div = ""
            elif gate.gate_type == "xor":
                line.append("(")
                line_end = ")"
                div = " ^ "

            first_child = True
            # Print children that are house events.
            for h_child in gate.h_children:
                if first_child:
                    line.append(h_child.name)
                    first_child = False
                else:
                    line.append(div + h_child.name)

            # Print children that are basic events.
            for b_child in gate.b_children:
                if first_child:
                    line.append(b_child.name)
                    first_child = False
                else:
                    line.append(div + b_child.name)

            # Print children that are gates.
            if not Settings.nested:
                for g_child in gate.g_children:
                    if first_child:
                        line.append(g_child.name)
                        first_child = False
                    else:
                        line.append(div + g_child.name)
            else:
                to_nest = [x for x in gate.g_children if not x.is_common()]
                not_nest = [x for x in gate.g_children if x.is_common()]
                for g_child in not_nest:
                    if first_child:
                        line.append(g_child.name)
                        first_child = False
                    else:
                        line.append(div + g_child.name)
                for g_child in to_nest:
                    if first_child:
                        write_formula(g_child, line)
                        first_child = False
                    else:
                        line.append(div)
                        write_formula(g_child, line)
                nested_gates.update(to_nest)

            line.append(line_end)

        line = [gate.name]
        line.append(" := ")
        write_formula(gate, line)
        o_file.write("".join(line))
        o_file.write("\n")

    sorted_gates = toposort_gates(top_event, Gate.gates)
    for gate in sorted_gates:
        if Settings.nested and gate in nested_gates:
            continue
        write_gate(gate, t_file)

    # Write basic events
    t_file.write("\n")
    for basic in BasicEvent.basic_events:
        t_file.write("p(" + basic.name + ") = " + str(basic.prob) + "\n")

    # Write house events
    t_file.write("\n")
    for house in HouseEvent.house_events:
        t_file.write("s(" + house.name + ") = " + str(house.state) + "\n")


def check_if_positive(desc, val):
    """Verifies that the value is positive or zero for the supplied argument.

    Args:
        desc: The description of the argument from the command-line.
        val: The value of the argument.

    Raises:
        ArgumentTypeError: The value is negative.
    """
    if val < 0:
        raise ap.ArgumentTypeError(desc + " is negative")


def check_if_less(desc, val, ref):
    """Verifies that the value is less than some reference for the argument.

    Args:
        desc: The description of the argument from the command-line.
        val: The value of the argument.
        ref: The reference value.

    Raises:
        ArgumentTypeError: The value is more than the reference.
    """
    if val > ref:
        raise ap.ArgumentTypeError(desc + " is more than " + str(ref))


def check_if_more(desc, val, ref):
    """Verifies that the value is more than some reference for the argument.

    Args:
        desc: The description of the argument from the command-line.
        val: The value of the argument.
        ref: The reference value.

    Raises:
        ArgumentTypeError: The value is less than the reference.
    """
    if val < ref:
        raise ap.ArgumentTypeError(desc + " is less than " + str(ref))


def manage_cmd_args():
    """Manages command-line description and arguments.

    Returns:
        Arguments that are collected from the command line.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
    """
    description = "Complex-Fault-Tree Generator"

    parser = ap.ArgumentParser(description=description,
                               formatter_class=ap.ArgumentDefaultsHelpFormatter)

    ft_name = "name for the fault tree"
    parser.add_argument("--ft-name", type=str, help=ft_name, metavar="NCNAME",
                        default="Autogenerated")

    root = "name for the root gate"
    parser.add_argument("--root", type=str, help=root, default="root",
                        metavar="NCNAME")

    seed = "seed of the pseudo-random number generator"
    parser.add_argument("--seed", type=int, help=seed, default=123,
                        metavar="int")

    basics = "number of basic events"
    parser.add_argument("-b", "--basics", type=int, help=basics, default=100,
                        metavar="int")

    children = "average number of children or arguments of gates"
    parser.add_argument("-c", "--children", type=float, help=children,
                        default=3.0, metavar="float")

    type_weights = "weights for sampling [AND, OR, K/N, NOT, XOR] gate types"
    parser.add_argument("--weights-g", type=str, nargs="+", help=type_weights,
                        default=[1, 1, 0, 0, 0], metavar="float")

    common_b = "percentage of common basic events per gate"
    parser.add_argument("--common-b", type=float, help=common_b, default=0.1,
                        metavar="float")

    common_g = "percentage of common gates per gate"
    parser.add_argument("--common-g", type=float, help=common_g, default=0.1,
                        metavar="float")

    parents_b = "average number of parents for common basic events"
    parser.add_argument("--parents-b", type=float, help=parents_b, default=2.0,
                        metavar="float")

    parents_g = "average number of parents for common gates"
    parser.add_argument("--parents-g", type=float, help=parents_g, default=2.0,
                        metavar="float")

    gates = "number of gates (discards parents-g and parents-b, common-b/g)"
    parser.add_argument("-g", "--gates", type=int, help=gates, default=0,
                        metavar="int")

    maxprob = "maximum probability for basic events"
    parser.add_argument("--maxprob", type=float, help=maxprob, default=0.1,
                        metavar="float")

    minprob = "minimum probability for basic events"
    parser.add_argument("--minprob", type=float, help=minprob, default=0.001,
                        metavar="float")

    house = "number of house events"
    parser.add_argument("--house", type=int, help=house, default=0,
                        metavar="int")

    ccf = "number of ccf groups"
    parser.add_argument("--ccf", type=int, help=ccf, default=0, metavar="int")

    out = "specify the output file to write the generated fault tree"
    parser.add_argument("-o", "--out", help=out, type=str,
                        default="fault_tree.xml", metavar="path")

    shorthand = "apply the shorthand format to the output"
    parser.add_argument("--shorthand", action="store_true", help=shorthand)

    nested = "join gates into a nested Boolean formula in the output"
    parser.add_argument("--nested", action="store_true", help=nested)

    args = parser.parse_args()

    # Check for validity of arguments
    check_if_positive(children, args.children)
    check_if_positive(basics, args.basics)
    check_if_positive(minprob, args.minprob)
    check_if_positive(maxprob, args.maxprob)
    check_if_positive(common_b, args.common_b)
    check_if_positive(common_g, args.common_g)
    check_if_positive(parents_b, args.parents_b)
    check_if_positive(parents_g, args.parents_g)
    check_if_positive(gates, args.gates)
    check_if_positive(house, args.house)
    check_if_positive(ccf, args.ccf)

    check_if_less(common_b, args.common_b, 0.9)
    check_if_less(common_g, args.common_g, 0.9)
    check_if_less(maxprob, args.maxprob, 1)
    check_if_less(minprob, args.minprob, 1)

    check_if_more(children, args.children, 2)
    check_if_more(parents_b, args.parents_b, 2)
    check_if_more(parents_g, args.parents_g, 2)
    return args


def validate_setup(args):
    """Checks if the relationships between arguments are valid for use.

    These checks are important
    to ensure that the requested fault tree is producible
    and realistic to achieve in reasonable time.

    Args:
        args: Command-line arguments with values.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
    """
    if args.maxprob < args.minprob:
        raise ap.ArgumentTypeError("Max probability < Min probability")

    if args.house >= args.basics:
        raise ap.ArgumentTypeError("Too many house events")

    if args.ccf > args.basics / args.children:
        raise ap.ArgumentTypeError("Too many ccf groups")

    if args.weights_g:
        if [i for i in args.weights_g if float(i) < 0]:
            raise ap.ArgumentTypeError("weights cannot be negative")

        if len(args.weights_g) > 5:
            raise ap.ArgumentTypeError("too many weights are provided")

        weights_float = [float(i) for i in args.weights_g]
        if sum(weights_float) == 0:
            raise ap.ArgumentTypeError("atleast one non-zero weight is needed")

        if len(weights_float) > 3 and not sum(weights_float[:3]):
            raise ap.ArgumentTypeError("cannot work with only XOR or NOT gates")

    if args.shorthand and args.out == "fault_tree.xml":
        args.out = "fault_tree.txt"

    if args.gates:
        # Check if there are enough gates for the basic events.
        if args.gates * args.children <= args.basics:
            raise ap.ArgumentTypeError("not enough gates and average children "
                                       "to achieve the number of basic events")


def setup_factors(args):
    """Configures the fault generation by assigning factors.

    Args:
        args: Command-line arguments with values for factors.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
    """
    validate_setup(args)
    random.seed(args.seed)
    Settings.seed = args.seed
    Settings.ft_name = args.ft_name
    Settings.root_name = args.root
    Settings.output = args.out
    Settings.nested = args.nested
    BasicEvent.min_prob = args.minprob
    BasicEvent.max_prob = args.maxprob
    Factors.num_basics = args.basics
    Factors.num_house = args.house
    Factors.num_ccf = args.ccf
    Factors.common_b = args.common_b
    Factors.common_g = args.common_g
    Factors.parents_b = args.parents_b
    Factors.parents_g = args.parents_g
    Factors.avg_children = args.children

    weights_float = [float(i) for i in args.weights_g]
    for i in range(5 - len(weights_float)):
        weights_float.append(0)
    Factors.set_weights(weights_float)

    if args.gates:
        Factors.constrain_num_gates(args.gates)

    Factors.calculate()


def main():
    """The main function of the fault tree generator.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
    """
    args = manage_cmd_args()
    setup_factors(args)

    top_event = generate_fault_tree()

    # Write output files
    if args.shorthand:
        write_shorthand(top_event)
    else:
        write_results(top_event)

if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as error:
        print("Argument Error:")
        print(error)
