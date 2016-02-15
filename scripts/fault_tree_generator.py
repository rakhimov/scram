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

from fault_tree import BasicEvent, HouseEvent, Gate, CcfGroup, FaultTree


class GeneratorFaultTree(FaultTree):
    """Specialization of a fault tree for generation purposes.

    The construction of fault tree members are handled through this object.
    It is assumed that no removal is going to happen after construction.
    """

    def __init__(self, name=None):
        """Initializes an empty fault tree.

        Args:
            name: The name of the system described by the fault tree container.
        """
        super(GeneratorFaultTree, self).__init__(name)

    def construct_gate(self):
        """Constructs a new gate.

        Returns:
            A fully initialized gate with random attributes.
        """
        gate = Gate("G" + str(len(self.gates) + 1),
                    Factors.get_random_operator())
        self.gates.append(gate)
        return gate

    def construct_basic_event(self):
        """Constructs a basic event with a unique identifier.

        Returns:
            A fully initialized basic event with a random probability.
        """
        basic_event = BasicEvent("B" + str(len(self.basic_events) + 1),
                                 random.uniform(Factors.min_prob,
                                                Factors.max_prob))
        self.basic_events.append(basic_event)
        return basic_event

    def construct_house_event(self):
        """Constructs a house event with a unique identifier.

        Returns:
            A fully initialized house event with a random state.
        """
        house_event = HouseEvent("H" + str(len(self.house_events) + 1),
                                 random.choice(["true", "false"]))
        self.house_events.append(house_event)
        return house_event

    def construct_ccf_group(self, members):
        """Constructs a unique CCF group with factors.

        Args:
            members: A list of member basic events.

        Returns:
            A fully initialized CCF group with random factors.
        """
        assert len(members) > 1
        ccf_group = CcfGroup("CCF" + str(len(self.ccf_groups) + 1))
        self.ccf_groups.append(ccf_group)
        ccf_group.members = members
        ccf_group.prob = random.uniform(Factors.min_prob, Factors.max_prob)
        ccf_group.model = "MGL"
        levels = random.randint(2, len(members))
        ccf_group.factors = [random.uniform(0.1, 1) for _ in range(levels - 1)]
        return ccf_group


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

    # Probabilistic factors
    min_prob = 0
    max_prob = 1

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
    __operators = ["and", "or", "atleast", "not", "xor"]

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
        assert len(weights) == len(Factors.__operators)
        assert sum(weights) > 0
        Factors.__weights_g = weights[:]
        Factors.__norm_weights = [x / sum(Factors.__weights_g)
                                  for x in Factors.__weights_g]
        Factors.__cum_dist = Factors.__norm_weights[:]
        Factors.__cum_dist.insert(0, 0)
        for i in range(1, len(Factors.__cum_dist)):
            Factors.__cum_dist[i] += Factors.__cum_dist[i - 1]

    @staticmethod
    def get_random_operator():
        """Samples the gate operator.

        Returns:
            A randomly chosen gate operator.
        """
        r_num = random.random()
        bin_num = 1
        while Factors.__cum_dist[bin_num] <= r_num:
            bin_num += 1
        return Factors.__operators[bin_num - 1]

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
        if gate.operator == "not":
            return 1
        elif gate.operator == "xor":
            return 2

        max_children = int(Factors.__max_children)
        # Dealing with the fractional part.
        if random.random() < (Factors.__max_children - max_children):
            max_children += 1

        if gate.operator == "atleast":
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


def init_gates(gates_queue, common_basics, common_gates, fault_tree):
    """Initializes gates and other basic events.

    Args:
        gates_queue: A deque of gates to be initialized.
        common_basics: A list of common basic events.
        common_gates: A list of common gates.
        fault_tree: The fault tree container of all events and constructs.
    """
    # Get an intermediate gate to initialize breadth-first
    gate = gates_queue.popleft()

    num_arguments = Factors.get_num_children(gate)

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

    while gate.num_arguments() < num_arguments:
        s_percent = random.random()  # sample percentage of gates
        s_common = random.random()  # sample the reuse frequency

        # Case when the number of basic events is already satisfied
        if len(fault_tree.basic_events) == Factors.num_basics:
            if not Factors.common_g and not Factors.common_b:
                # Reuse already initialized basic events
                gate.add_argument(random.choice(fault_tree.basic_events))
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
                    if random_gate in gate.g_arguments or random_gate is gate:
                        continue
                    if (not random_gate.g_arguments or
                            random_gate not in ancestors):
                        if not random_gate.parents:
                            gates_queue.append(random_gate)
                        gate.add_argument(random_gate)
                        break
            else:
                new_gate = fault_tree.construct_gate()
                gate.add_argument(new_gate)
                gates_queue.append(new_gate)
        else:
            # Create a new basic event or use a common one
            if s_common < Factors.common_b and common_basics:
                orphans = [x for x in common_basics if not x.parents]
                if orphans:
                    gate.add_argument(random.choice(orphans))
                    continue

                single_parent = [x for x in common_basics
                                 if len(x.parents) == 1]
                if single_parent:
                    gate.add_argument(random.choice(single_parent))
                else:
                    gate.add_argument(random.choice(common_basics))
            else:
                gate.add_argument(fault_tree.construct_basic_event())

    # Corner case when not enough new basic events initialized, but
    # there are no more intermediate gates to use due to a big ratio
    # or just random accident.
    if not gates_queue and len(fault_tree.basic_events) < Factors.num_basics:
        # Initialize more gates by randomly choosing places in the
        # fault tree.
        random_gate = random.choice(fault_tree.gates)
        while (random_gate.operator == "not" or
               random_gate.operator == "xor" or
               random_gate in common_gates):
            random_gate = random.choice(fault_tree.gates)
        new_gate = fault_tree.construct_gate()
        random_gate.add_argument(new_gate)
        gates_queue.append(new_gate)


def generate_fault_tree(ft_name, root_name):
    """Generates a fault tree of specified complexity.

    The Factors class attributes are used as parameters for complexity.

    Args:
        ft_name: The name of the fault tree.
        root_name: The name for the root gate of the fault tree.

    Returns:
        Top gate of the created fault tree.
    """
    fault_tree = GeneratorFaultTree(ft_name)
    # Start with a top event
    top_event = fault_tree.construct_gate()
    fault_tree.top_gate = top_event
    while top_event.operator == "xor" or top_event.operator == "not":
        top_event.operator = Factors.get_random_operator()
    top_event.name = root_name

    # Estimating the parameters
    num_gates = Factors.get_num_gates()
    num_common_basics = Factors.get_num_common_basics(num_gates)
    num_common_gates = Factors.get_num_common_gates(num_gates)
    common_basics = [fault_tree.construct_basic_event()
                     for _ in range(num_common_basics)]
    common_gates = [fault_tree.construct_gate()
                    for _ in range(num_common_gates)]

    # Container for not yet initialized gates
    # A deque is used to traverse the tree breadth-first
    gates_queue = deque()
    gates_queue.append(top_event)

    # Proceed with argument gates
    while gates_queue:
        init_gates(gates_queue, common_basics, common_gates, fault_tree)

    assert(not [x for x in fault_tree.basic_events if x.is_orphan()])
    assert(not [x for x in fault_tree.gates
                if x.is_orphan() and x is not top_event])

    # Distribute house events
    while len(fault_tree.house_events) < Factors.num_house:
        target_gate = random.choice(fault_tree.gates)
        if (target_gate is not top_event and
                target_gate.operator != "xor" and
                target_gate.operator != "not"):
            target_gate.add_argument(fault_tree.construct_house_event())

    # Create CCF groups from the existing basic events.
    if Factors.num_ccf:
        members = fault_tree.basic_events[:]
        random.shuffle(members)
        first_mem = 0
        last_mem = 0
        while len(fault_tree.ccf_groups) < Factors.num_ccf:
            group_size = random.randint(2, int(2 * Factors.avg_children - 2))
            last_mem = first_mem + group_size
            if last_mem > len(members):
                break
            fault_tree.construct_ccf_group(members[first_mem:last_mem])
            first_mem = last_mem
        fault_tree.non_ccf_events = members[first_mem:]

    return fault_tree


def write_info(fault_tree, tree_file, seed):
    """Writes the information about the setup for fault tree generation.

    Args:
        fault_tree: A full, valid, well-formed fault tree.
        tree_file: A file open for writing.
        seed: The seed of the pseudo-random number generator.
    """
    tree_file.write("<?xml version=\"1.0\"?>\n")
    tree_file.write(
        "<!--\nThis is a description of the auto-generated fault tree\n"
        "with the following parameters:\n\n"
        "The output file name: " + tree_file.name + "\n"
        "The fault tree name: " + fault_tree.name + "\n"
        "The root gate name: " + fault_tree.top_gate.name + "\n\n"
        "The seed of the random number generator: " + str(seed) + "\n"
        "The number of basic events: " + str(Factors.num_basics) + "\n"
        "The number of house events: " + str(Factors.num_house) + "\n"
        "The number of CCF groups: " + str(Factors.num_ccf) + "\n"
        "The average number of gate arguments: " +
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
        str(Factors.max_prob) + "\n"
        "Minimum probability for basic events: " +
        str(Factors.min_prob) + "\n"
        "-->\n")


def get_size_summary(fault_tree):
    """Gathers information about the size of the fault tree.

    Args:
        fault_tree: A full, valid, well-formed fault tree.

    Returns:
        A text snippet to be embedded in a XML summary.
    """
    and_gates = [x for x in fault_tree.gates if x.operator == "and"]
    or_gates = [x for x in fault_tree.gates if x.operator == "or"]
    atleast_gates = [x for x in fault_tree.gates if x.operator == "atleast"]
    not_gates = [x for x in fault_tree.gates if x.operator == "not"]
    xor_gates = [x for x in fault_tree.gates if x.operator == "xor"]
    return (
        "The number of basic events: %d" % len(fault_tree.basic_events) + "\n"
        "The number of house events: %d" % len(fault_tree.house_events) + "\n"
        "The number of CCF groups: %d" % len(fault_tree.ccf_groups) + "\n"
        "The number of gates: %d" % len(fault_tree.gates) + "\n"
        "    AND gates: %d" % len(and_gates) + "\n"
        "    OR gates: %d" % len(or_gates) + "\n"
        "    K/N gates: %d" % len(atleast_gates) + "\n"
        "    NOT gates: %d" % len(not_gates) + "\n"
        "    XOR gates: %d" % len(xor_gates) + "\n")


def calculate_complexity_factors(fault_tree):
    """Computes complexity factors of the generated fault tree.

    Args:
        fault_tree: A full, valid, well-formed fault tree.

    Returns:
        frac_b: fraction of basic events in arguments per gate
        common_b: fraction of common basic events in basic events per gate
        common_g: fraction of common gates in gates per gate
    """
    frac_b = 0
    common_b = 0
    common_g = 0
    for gate in fault_tree.gates:
        num_b_arguments = len(gate.b_arguments)
        num_g_arguments = len(gate.g_arguments)
        frac_b += num_b_arguments / (num_g_arguments + num_b_arguments)
        if gate.b_arguments:
            num_common_b = len([x for x in gate.b_arguments if x.is_common()])
            common_b += num_common_b / num_b_arguments
        if gate.g_arguments:
            num_common_g = len([x for x in gate.g_arguments if x.is_common()])
            common_g += num_common_g / num_g_arguments
    common_b /= len([x for x in fault_tree.gates if x.b_arguments])
    common_g /= len([x for x in fault_tree.gates if x.g_arguments])
    frac_b /= len(fault_tree.gates)
    return frac_b, common_b, common_g


def get_complexity_summary(fault_tree):
    """Gathers information about the complexity factors of the fault tree.

    Args:
        fault_tree: A full, valid, well-formed fault tree.

    Returns:
        A text snippet to be embedded in a XML summary.
    """
    frac_b, common_b, common_g = calculate_complexity_factors(fault_tree)
    shared_b = [x for x in fault_tree.basic_events if x.is_common()]
    shared_g = [x for x in fault_tree.gates if x.is_common()]
    summary_txt = (
        "Basic events to gates ratio: %f" %
        (len(fault_tree.basic_events) / len(fault_tree.gates)) + "\n"
        "The average number of gate arguments: %f" %
        (sum(x.num_arguments() for x in fault_tree.gates) /
         len(fault_tree.gates)) + "\n"
        "The number of common basic events: %d" % len(shared_b) + "\n"
        "The number of common gates: %d" % len(shared_g) + "\n"
        "Percentage of common basic events per gate: %f" % common_b + "\n"
        "Percentage of common gates per gate: %f" % common_g + "\n"
        "Percentage of arguments that are basic events per gate: %f" %
        frac_b + "\n")
    if shared_b:
        summary_txt += (
            "The avg. number of parents for common basic events: %f" %
            (sum(x.num_parents() for x in shared_b) / len(shared_b)) + "\n")
    if shared_g:
        summary_txt += (
            "The avg. number of parents for common gates: %f" %
            (sum(x.num_parents() for x in shared_g) / len(shared_g)) + "\n")
    return summary_txt


def write_summary(fault_tree, tree_file):
    """Writes the summary of the generated fault tree.

    Args:
        fault_tree: A full, valid, well-formed fault tree.
        tree_file: A file open for writing.
    """
    tree_file.write(
        "<!--\nThe generated fault tree has the following metrics:\n\n")
    tree_file.write(get_size_summary(fault_tree))
    tree_file.write(get_complexity_summary(fault_tree))
    tree_file.write("-->\n\n")


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

    nest = "nestedness for Boolean formulae in the XML output"
    parser.add_argument("--nest", type=int, help=nest, default=0, metavar="int")

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
    check_if_positive(nest, args.nest)

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

    if args.shorthand:
        if args.out == "fault_tree.xml":
            args.out = "fault_tree.txt"
        if args.nest > 0:
            raise ap.ArgumentTypeError("no support for nested formulae "
                                       "in the shorthand format")

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
    Factors.min_prob = args.minprob
    Factors.max_prob = args.maxprob
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

    fault_tree = generate_fault_tree(args.ft_name, args.root)

    with open(args.out, "w") as tree_file:
        if args.shorthand:
            tree_file.write(fault_tree.to_shorthand())
        else:
            write_info(fault_tree, tree_file, args.seed)
            write_summary(fault_tree, tree_file)
            tree_file.write(fault_tree.to_xml(args.nest))

if __name__ == "__main__":
    try:
        main()
    except ap.ArgumentTypeError as error:
        print("Argument Error:")
        print(error)
