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

    O(N) + O((N/Ratio)^2*exp(-NumArgs/Ratio)) + O(CommonG*exp(CommonB))

Where N is the number of basic events,
and Ratio is N / num_gate.

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
import sys

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

    def construct_top_gate(self, root_name):
        """Constructs and assigns a new gate suitable for being a root.

        Args:
            root_name: Unique name for the root gate.
        """
        assert not self.top_gate and not self.top_gates
        operator = Factors.get_random_operator()
        while operator == "xor" or operator == "not":
            operator = Factors.get_random_operator()
        self.top_gate = Gate(root_name, operator)
        self.gates.append(self.top_gate)

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


class FactorError(Exception):
    """Errors in configuring factors for the fault tree generation."""

    pass


class Factors(object):
    """Collection of factors that determine the complexity of the fault tree.

    This collection must be setup and updated
    before the fault tree generation processes.

    Attributes:
        num_args: The average number of arguments for gates.
        num_basic: The number of basic events.
        num_house: The number of house events.
        num_ccf: The number of ccf groups.
        common_b: The percentage of common basic events per gate.
        common_g: The percentage of common gates per gate.
        parents_b: The average number of parents for common basic events.
        parents_g: The average number of parents for common gates.
    """

    # Probabilistic factors
    min_prob = 0
    max_prob = 1

    # Configurable graph factors
    num_basic = None
    num_house = None
    num_ccf = None
    common_b = None
    common_g = None
    num_args = None
    parents_b = None
    parents_g = None
    __weights_g = None  # should not be set directly

    # Constant configurations
    __OPERATORS = ["and", "or", "atleast", "not", "xor"]  # the order matters

    # Calculated factors
    __norm_weights = []  # normalized weights
    __cum_dist = []  # CDF from the weights of the gate types
    __max_args = None  # the upper bound for the number of arguments
    __ratio = None  # basic events to gates ratio per gate
    __percent_basic = None  # percentage of basic events in gate arguments
    __percent_gate = None  # percentage of gates in gate arguments

    # Special case with the constrained number of gates
    __num_gate = None  # If set, all other factors get affected.

    @staticmethod
    def set_min_max_prob(min_value, max_value):
        """Sets the probability boundaries for basic events.

        Args:
            min_value: The lower inclusive boundary.
            max_value: The upper inclusive boundary.

        Raises:
            FactorError: Invalid values or setup.
        """
        if min_value < 0 or min_value > 1:
            raise FactorError("Min probability must be in [0, 1] range.")
        if max_value < 0 or max_value > 1:
            raise FactorError("Max probability must be in [0, 1] range.")
        if min_value > max_value:
            raise FactorError("Min probability > Max probability.")
        Factors.min_prob = min_value
        Factors.max_prob = max_value

    @staticmethod
    def set_common_event_factors(common_b, common_g, parents_b, parents_g):
        """Sets the factors for the number of common events.

        Args:
            common_b: The percentage of common basic events per gate.
            common_g: The percentage of common gates per gate.
            parents_b: The average number of parents for common basic events.
            parents_g: The average number of parents for common gates.

        Raises:
            FactorError: Invalid values or setup.
        """
        max_common = 0.9  # a practical limit (not a formal constraint)
        if common_b <= 0 or common_b > max_common:
            raise FactorError("common_b not in (0, " + str(max_common) + "].")
        if common_g <= 0 or common_g > max_common:
            raise FactorError("common_g not in (0, " + str(max_common) + "].")
        max_parent = 100  # also a practical limit
        if parents_b < 2 or parents_b > max_parent:
            raise FactorError("parents_b not in [2, " + str(max_parent) + "].")
        if parents_g < 2 or parents_g > max_parent:
            raise FactorError("parents_g not in [2, " + str(max_parent) + "].")
        Factors.common_b = common_b
        Factors.common_g = common_g
        Factors.parents_b = parents_b
        Factors.parents_g = parents_g

    @staticmethod
    def set_num_factors(num_args, num_basic, num_house=0, num_ccf=0):
        """Sets the size factors.

        Args:
            num_args: The average number of arguments for gates.
            num_basic: The number of basic events.
            num_house: The number of house events.
            num_ccf: The number of ccf groups.

        Raises:
            FactorError: Invalid values or setup.
        """
        if num_args < 2:
            raise FactorError("avg. # of gate arguments can't be less than 2.")
        if num_basic < 1:
            raise FactorError("# of basic events must be more than 0.")
        if num_house < 0:
            raise FactorError("# of house events can't be negative.")
        if num_ccf < 0:
            raise FactorError("# of CCF groups can't be negative.")
        if num_house >= num_basic:
            raise FactorError("Too many house events.")
        if num_ccf > num_basic / num_args:
            raise FactorError("Too many CCF groups.")
        Factors.num_args = num_args
        Factors.num_basic = num_basic
        Factors.num_house = num_house
        Factors.num_ccf = num_ccf

    @staticmethod
    def __calculate_max_args(num_args, weights):
        """Calculates the maximum number of arguments for sampling.

        The result may have a fractional part
        that must be adjusted in sampling accordingly.

        Args:
            num_args: The average number of arguments for gates.
            weights: Normalized weights for gate types.

        Returns:
            The upper bound for sampling in symmetric distributions.
        """
        # Min numbers for AND, OR, K/N, NOT, XOR types.
        min_args = [2, 2, 3, 1, 2]
        # Note that max and min numbers are the same for NOT and XOR.
        const_args = min_args[3:]
        const_weights = weights[3:]
        const_contrib = [x * y for x, y in zip(const_args, const_weights)]
        # AND, OR, K/N gate types can have the varying number of args.
        var_args = min_args[:3]
        var_weights = weights[:3]
        var_contrib = [x * y for x, y in zip(var_args, var_weights)]

        # AND, OR, K/N gate types can have the varying number of arguments.
        # Since the distribution is symmetric, the average is (max + min) / 2.
        return ((2 * num_args - sum(var_contrib) - 2 * sum(const_contrib)) /
                sum(var_weights))

    @staticmethod
    def calculate():
        """Calculates any derived factors from the setup.

        This function must be called after all public factors are initialized.
        """
        Factors.__max_args = Factors.__calculate_max_args(
            Factors.num_args,
            Factors.__norm_weights)
        g_factor = 1 - Factors.common_g + Factors.common_g / Factors.parents_g
        Factors.__ratio = Factors.num_args * g_factor - 1
        Factors.__percent_basic = Factors.__ratio / (1 + Factors.__ratio)
        Factors.__percent_gate = 1 / (1 + Factors.__ratio)

    @staticmethod
    def get_gate_weights():
        """Provides weights for gate types.

        Returns:
            Expected to return weights from the arguments.
        """
        assert Factors.__weights_g is not None
        return Factors.__weights_g

    @staticmethod
    def set_gate_weights(weights):
        """Updates gate type weights.

        Args:
            weights: Weights of gate types.
                The weights must have the same order as in OPERATORS list.
                If weights for some operators are missing,
                they are assumed to be 0.

        Raises:
            FactorError: Invalid weight values or setup.
        """
        if not weights:
            raise FactorError("No weights are provided")
        if [i for i in weights if i < 0]:
            raise FactorError("Weights cannot be negative")
        if len(weights) > len(Factors.__OPERATORS):
            raise FactorError("Too many weights are provided")
        if sum(weights) == 0:
            raise FactorError("At least one non-zero weight is needed")
        if len(weights) > 3 and not sum(weights[:3]):
            raise FactorError("Cannot work with only XOR or NOT gates")

        Factors.__weights_g = weights[:]
        for _ in range(len(Factors.__OPERATORS) - len(weights)):
            Factors.__weights_g.append(0)  # padding for missing weights
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
        return Factors.__OPERATORS[bin_num - 1]

    @staticmethod
    def get_num_args(gate):
        """Randomly selects the number of arguments for the given gate type.

        This function has a side effect.
        It sets k_num for the K/N type of gates
        depending on the number of arguments.

        Args:
            gate: The parent gate for arguments.

        Returns:
            Random number of arguments.
        """
        if gate.operator == "not":
            return 1
        elif gate.operator == "xor":
            return 2

        max_args = int(Factors.__max_args)
        # Dealing with the fractional part.
        if random.random() < (Factors.__max_args - max_args):
            max_args += 1

        if gate.operator == "atleast":
            if max_args < 3:
                max_args = 3
            num_args = random.randint(3, max_args)
            gate.k_num = random.randint(2, num_args - 1)
            return num_args

        return random.randint(2, max_args)

    @staticmethod
    def get_percent_gate():
        """Returns the percentage of gates that should be in arguments."""
        return Factors.__percent_gate

    @staticmethod
    def get_num_gate():
        """Approximates the number of gates in the resulting fault tree.

        This is an estimate of the number of gates
        needed to initialize the fault tree
        with the given number of basic events
        and fault tree properties.

        Returns:
            The number of gates needed for the given basic events.
        """
        # Special case of constrained gates
        if Factors.__num_gate:
            return Factors.__num_gate
        b_factor = 1 - Factors.common_b + Factors.common_b / Factors.parents_b
        return int(Factors.num_basic /
                   (Factors.__percent_basic * Factors.num_args * b_factor))

    @staticmethod
    def get_num_common_basics(num_gate):
        """Estimates the number of common basic events.

        These common basic events must be chosen
        from the total number of basic events
        in order to ensure the correct average number of parents.

        Args:
            num_gate: The total number of gates in the future fault tree

        Returns:
            The estimated number of common basic events.
        """
        return int(Factors.common_b * Factors.__percent_basic *
                   Factors.num_args * num_gate / Factors.parents_b)

    @staticmethod
    def get_num_common_gates(num_gate):
        """Estimates the number of common gates.

        These common gates must be chosen
        from the total number of gates
        in order to ensure the correct average number of parents.

        Args:
            num_gate: The total number of gates in the future fault tree

        Returns:
            The estimated number of common gates.
        """
        return int(Factors.common_g * Factors.__percent_gate *
                   Factors.num_args * num_gate / Factors.parents_g)

    @staticmethod
    def calculate_parents_g(num_gate):
        """Estimates the average number of parents for gates.

        This function makes possible to generate a fault tree
        with the user specified number of gates.
        All other parameters
        except for the number of parents
        must be set for use in calculations.

        Args:
            num_gate: The total number of gates in the future fault tree

        Returns:
            The estimated average number of parents of gates.
        """
        b_factor = 1 - Factors.common_b + Factors.common_b / Factors.parents_b
        ratio = 1 / (num_gate / Factors.num_basic *
                     Factors.num_args * b_factor - 1)
        assert ratio > 0
        parents = Factors.common_g / (Factors.common_g - 1 +
                                      (1 + ratio) / Factors.num_args)
        if parents < 2:
            parents = 2
        return parents

    @staticmethod
    def constrain_num_gate(num_gate):
        """Constrains the number of gates.

        The number of parents and the ratios for common nodes are manipulated.

        Args:
            num_gate: The total number of gates in the future fault tree
        """
        if num_gate < 1:
            raise FactorError("# of gates can't be less than 1.")
        if num_gate * Factors.num_args <= Factors.num_basic:
            raise FactorError("Not enough gates and avg. # of args "
                              "to achieve the # of basic events")
        Factors.__num_gate = num_gate
        # Calculate the ratios
        alpha = Factors.__num_gate / Factors.num_basic
        common = max(Factors.common_g, Factors.common_b)
        min_common = 1 - (1 + alpha) / Factors.num_args / alpha
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


def candidate_gates(common_gates):
    """Lazy generator of candidates for common gates.

    Args:
        common_gates: A list of common gates.

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


def correct_for_exhaustion(gates_queue, common_gates, fault_tree):
    """Corrects the generation for queue exhaustion.

    Corner case when not enough new basic events initialized,
    but there are no more intermediate gates to use
    due to a big ratio or just random accident.

    Args:
        gates_queue: A deque of gates to be initialized.
        common_gates: A list of common gates.
        fault_tree: The fault tree container of all events and constructs.
    """
    if not gates_queue and len(fault_tree.basic_events) < Factors.num_basic:
        # Initialize one more gate
        # by randomly choosing places in the fault tree.
        random_gate = random.choice(fault_tree.gates)
        while (random_gate.operator == "not" or
               random_gate.operator == "xor" or
               random_gate in common_gates):
            random_gate = random.choice(fault_tree.gates)
        new_gate = fault_tree.construct_gate()
        random_gate.add_argument(new_gate)
        gates_queue.append(new_gate)


def choose_basic_event(s_common, common_basics, fault_tree):
    """Creates a new basic event or uses a common one for gate arguments.

    Args:
        s_common: Sampled factor to choose common basic events.
        common_basics: A list of common basic events to choose from.
        fault_tree: The fault tree container of all events and constructs.

    Returns:
        Basic event argument for a gate.
    """
    if s_common < Factors.common_b and common_basics:
        orphans = [x for x in common_basics if not x.parents]
        if orphans:
            return random.choice(orphans)

        single_parent = [x for x in common_basics if len(x.parents) == 1]
        if single_parent:
            return random.choice(single_parent)

        return random.choice(common_basics)
    else:
        return fault_tree.construct_basic_event()


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

    num_arguments = Factors.get_num_args(gate)

    ancestors = None  # needed for cycle prevention
    max_tries = len(common_gates)  # the number of maximum tries
    num_trials = 0  # the number of trials to get a common gate

    while gate.num_arguments() < num_arguments:
        s_percent = random.random()  # sample percentage of gates
        s_common = random.random()  # sample the reuse frequency

        # Case when the number of basic events is already satisfied
        if len(fault_tree.basic_events) == Factors.num_basic:
            s_common = 0  # use only common nodes

        if s_percent < Factors.get_percent_gate():
            # Create a new gate or use a common one
            if s_common < Factors.common_g and num_trials < max_tries:
                # Lazy evaluation of ancestors
                if not ancestors:
                    ancestors = gate.get_ancestors()

                for random_gate in candidate_gates(common_gates):
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
            gate.add_argument(choose_basic_event(s_common, common_basics,
                                                 fault_tree))

    correct_for_exhaustion(gates_queue, common_gates, fault_tree)


def distribute_house_events(fault_tree):
    """Distributes house events to already initialized gates.

    Args:
        fault_tree: The fault tree container of all events and constructs.
    """
    while len(fault_tree.house_events) < Factors.num_house:
        target_gate = random.choice(fault_tree.gates)
        if (target_gate is not fault_tree.top_gate and
                target_gate.operator != "xor" and
                target_gate.operator != "not"):
            target_gate.add_argument(fault_tree.construct_house_event())


def generate_ccf_groups(fault_tree):
    """Creates CCF groups from the existing basic events.

    Args:
        fault_tree: The fault tree container of all events and constructs.
    """
    if Factors.num_ccf:
        members = fault_tree.basic_events[:]
        random.shuffle(members)
        first_mem = 0
        last_mem = 0
        while len(fault_tree.ccf_groups) < Factors.num_ccf:
            group_size = random.randint(2, int(2 * Factors.num_args - 2))
            last_mem = first_mem + group_size
            if last_mem > len(members):
                break
            fault_tree.construct_ccf_group(members[first_mem:last_mem])
            first_mem = last_mem
        fault_tree.non_ccf_events = members[first_mem:]


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
    fault_tree.construct_top_gate(root_name)

    # Estimating the parameters
    num_gate = Factors.get_num_gate()
    num_common_basics = Factors.get_num_common_basics(num_gate)
    num_common_gates = Factors.get_num_common_gates(num_gate)
    common_basics = [fault_tree.construct_basic_event()
                     for _ in range(num_common_basics)]
    common_gates = [fault_tree.construct_gate()
                    for _ in range(num_common_gates)]

    # Container for not yet initialized gates
    # A deque is used to traverse the tree breadth-first
    gates_queue = deque()
    gates_queue.append(fault_tree.top_gate)
    while gates_queue:
        init_gates(gates_queue, common_basics, common_gates, fault_tree)

    assert(not [x for x in fault_tree.basic_events if x.is_orphan()])
    assert(not [x for x in fault_tree.gates
                if x.is_orphan() and x is not fault_tree.top_gate])

    distribute_house_events(fault_tree)
    generate_ccf_groups(fault_tree)
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
        "The number of basic events: " + str(Factors.num_basic) + "\n"
        "The number of house events: " + str(Factors.num_house) + "\n"
        "The number of CCF groups: " + str(Factors.num_ccf) + "\n"
        "The average number of gate arguments: " +
        str(Factors.num_args) + "\n"
        "The weights of gate types [AND, OR, K/N, NOT, XOR]: " +
        str(Factors.get_gate_weights()) + "\n"
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


def manage_cmd_args():
    """Manages command-line description and arguments.

    Returns:
        Arguments that are collected from the command line.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
    """
    parser = ap.ArgumentParser(description="Complex-Fault-Tree Generator",
                               formatter_class=ap.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--ft-name", type=str, help="name for the fault tree",
                        metavar="NCNAME", default="Autogenerated")
    parser.add_argument("--root", type=str, help="name for the root gate",
                        default="root", metavar="NCNAME")
    parser.add_argument("--seed", type=int, default=123, metavar="int",
                        help="seed for the PRNG")
    parser.add_argument("-b", "--num-basic", type=int, help="# of basic events",
                        default=100, metavar="int")
    parser.add_argument("-a", "--num-args", type=float, default=3.0,
                        help="avg. # of gate arguments", metavar="float")
    parser.add_argument("--weights-g", type=str, nargs="+", metavar="float",
                        help="weights for [AND, OR, K/N, NOT, XOR] gates",
                        default=[1, 1, 0, 0, 0])
    parser.add_argument("--common-b", type=float, default=0.1, metavar="float",
                        help="avg. %% of common basic events per gate")
    parser.add_argument("--common-g", type=float, default=0.1, metavar="float",
                        help="avg. %% of common gates per gate")
    parser.add_argument("--parents-b", type=float, default=2, metavar="float",
                        help="avg. # of parents for common basic events")
    parser.add_argument("--parents-g", type=float, default=2, metavar="float",
                        help="avg. # of parents for common gates")
    parser.add_argument("-g", "--num-gate", type=int, default=0, metavar="int",
                        help="# of gates (discards parents-b/g and common-b/g)")
    parser.add_argument("--max-prob", type=float, default=0.1, metavar="float",
                        help="maximum probability for basic events")
    parser.add_argument("--min-prob", type=float, default=0.01, metavar="float",
                        help="minimum probability for basic events")
    parser.add_argument("--num-house", type=int, help="# of house events",
                        default=0, metavar="int")
    parser.add_argument("--num-ccf", type=int, help="# of ccf groups",
                        default=0, metavar="int")
    parser.add_argument("-o", "--out", type=str, default="fault_tree.xml",
                        metavar="path", help="a file to write the fault tree")
    parser.add_argument("--shorthand", action="store_true",
                        help="apply the shorthand format to the output")
    parser.add_argument("--nest", type=int, default=0, metavar="int",
                        help="nestedness of Boolean formulae in the XML output")
    args = parser.parse_args()
    if args.nest < 0:
        raise ap.ArgumentTypeError("The nesting factor cannot be negative")
    if args.shorthand:
        if args.out == "fault_tree.xml":
            args.out = "fault_tree.txt"
        if args.nest > 0:
            raise ap.ArgumentTypeError("No support for nested formulae "
                                       "in the shorthand format")
    return args


def setup_factors(args):
    """Configures the fault generation by assigning factors.

    Args:
        args: Command-line arguments with values for factors.

    Raises:
        ArgumentTypeError: Problems with the arguments.
        FactorError: Invalid setup for factors.
    """
    random.seed(args.seed)
    Factors.set_min_max_prob(args.min_prob, args.max_prob)
    Factors.set_common_event_factors(args.common_b, args.common_g,
                                     args.parents_b, args.parents_g)
    Factors.set_num_factors(args.num_args, args.num_basic, args.num_house,
                            args.num_ccf)
    Factors.set_gate_weights([float(i) for i in args.weights_g])
    if args.num_gate:
        Factors.constrain_num_gate(args.num_gate)
    Factors.calculate()


def main():
    """The main function of the fault tree generator.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
        FactorError: Invalid setup for factors.
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
    except ap.ArgumentTypeError as err:
        print("Argument Error:\n" + str(err))
        sys.exit(2)
    except FactorError as err:
        print("Error in factors:\n" + str(err))
        sys.exit(1)
