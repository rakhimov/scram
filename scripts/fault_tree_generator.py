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

The generated fault tree can be put into an XML file with the Open-PSA MEF
or the Aralia format file.
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

from __future__ import print_function, division, absolute_import

from collections import deque
import random
import sys

import argparse as ap

from fault_tree import BasicEvent, HouseEvent, Gate, CcfGroup, FaultTree


class FactorError(Exception):
    """Errors in configuring factors for the fault tree generation."""

    pass


class Factors(object):  # pylint: disable=too-many-instance-attributes
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

    # Constant configurations
    __OPERATORS = ["and", "or", "atleast", "not", "xor"]  # the order matters

    def __init__(self):
        """Partial constructor."""
        # Probabilistic factors
        self.min_prob = 0
        self.max_prob = 1

        # Configurable graph factors
        self.num_basic = None
        self.num_house = None
        self.num_ccf = None
        self.common_b = None
        self.common_g = None
        self.num_args = None
        self.parents_b = None
        self.parents_g = None
        self.__weights_g = None  # should not be set directly

        # Calculated factors
        self.__norm_weights = []  # normalized weights
        self.__cum_dist = []  # CDF from the weights of the gate types
        self.__max_args = None  # the upper bound for the number of arguments
        self.__ratio = None  # basic events to gates ratio per gate
        self.__percent_basic = None  # % of basic events in gate arguments
        self.__percent_gate = None  # % of gates in gate arguments

        # Special case with the constrained number of gates
        self.__num_gate = None  # If set, all other factors get affected.

    def set_min_max_prob(self, min_value, max_value):
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
        self.min_prob = min_value
        self.max_prob = max_value

    def set_common_event_factors(self, common_b, common_g, parents_b,
                                 parents_g):
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
        self.common_b = common_b
        self.common_g = common_g
        self.parents_b = parents_b
        self.parents_g = parents_g

    def set_num_factors(self, num_args, num_basic, num_house=0, num_ccf=0):
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
        self.num_args = num_args
        self.num_basic = num_basic
        self.num_house = num_house
        self.num_ccf = num_ccf

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

    def calculate(self):
        """Calculates any derived factors from the setup.

        This function must be called after all public factors are initialized.
        """
        self.__max_args = Factors.__calculate_max_args(self.num_args,
                                                       self.__norm_weights)
        g_factor = 1 - self.common_g + self.common_g / self.parents_g
        self.__ratio = self.num_args * g_factor - 1
        self.__percent_basic = self.__ratio / (1 + self.__ratio)
        self.__percent_gate = 1 / (1 + self.__ratio)

    def get_gate_weights(self):
        """Provides weights for gate types.

        Returns:
            Expected to return weights from the arguments.
        """
        assert self.__weights_g is not None
        return self.__weights_g

    def set_gate_weights(self, weights):
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

        self.__weights_g = weights[:]
        for _ in range(len(Factors.__OPERATORS) - len(weights)):
            self.__weights_g.append(0)  # padding for missing weights
        self.__norm_weights = [x / sum(self.__weights_g)
                               for x in self.__weights_g]
        self.__cum_dist = self.__norm_weights[:]
        self.__cum_dist.insert(0, 0)
        for i in range(1, len(self.__cum_dist)):
            self.__cum_dist[i] += self.__cum_dist[i - 1]

    def get_random_operator(self):
        """Samples the gate operator.

        Returns:
            A randomly chosen gate operator.
        """
        r_num = random.random()
        bin_num = 1
        while self.__cum_dist[bin_num] <= r_num:
            bin_num += 1
        return Factors.__OPERATORS[bin_num - 1]

    def get_num_args(self, gate):
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

        max_args = int(self.__max_args)
        # Dealing with the fractional part.
        if random.random() < (self.__max_args - max_args):
            max_args += 1

        if gate.operator == "atleast":
            if max_args < 3:
                max_args = 3
            num_args = random.randint(3, max_args)
            gate.k_num = random.randint(2, num_args - 1)
            return num_args

        return random.randint(2, max_args)

    def get_percent_gate(self):
        """Returns the percentage of gates that should be in arguments."""
        return self.__percent_gate

    def get_num_gate(self):
        """Approximates the number of gates in the resulting fault tree.

        This is an estimate of the number of gates
        needed to initialize the fault tree
        with the given number of basic events
        and fault tree properties.

        Returns:
            The number of gates needed for the given basic events.
        """
        # Special case of constrained gates
        if self.__num_gate:
            return self.__num_gate
        b_factor = 1 - self.common_b + self.common_b / self.parents_b
        return int(self.num_basic /
                   (self.__percent_basic * self.num_args * b_factor))

    def get_num_common_basic(self, num_gate):
        """Estimates the number of common basic events.

        These common basic events must be chosen
        from the total number of basic events
        in order to ensure the correct average number of parents.

        Args:
            num_gate: The total number of gates in the future fault tree

        Returns:
            The estimated number of common basic events.
        """
        return int(self.common_b * self.__percent_basic *
                   self.num_args * num_gate / self.parents_b)

    def get_num_common_gate(self, num_gate):
        """Estimates the number of common gates.

        These common gates must be chosen
        from the total number of gates
        in order to ensure the correct average number of parents.

        Args:
            num_gate: The total number of gates in the future fault tree

        Returns:
            The estimated number of common gates.
        """
        return int(self.common_g * self.__percent_gate *
                   self.num_args * num_gate / self.parents_g)

    def constrain_num_gate(self, num_gate):
        """Constrains the number of gates.

        The number of parents and the ratios for common nodes are manipulated.

        Args:
            num_gate: The total number of gates in the future fault tree
        """
        if num_gate < 1:
            raise FactorError("# of gates can't be less than 1.")
        if num_gate * self.num_args <= self.num_basic:
            raise FactorError("Not enough gates and avg. # of args "
                              "to achieve the # of basic events")
        self.__num_gate = num_gate
        # Calculate the ratios
        alpha = self.__num_gate / self.num_basic
        common = max(self.common_g, self.common_b)
        min_common = 1 - (1 + alpha) / self.num_args / alpha
        if common < min_common:
            common = round(min_common + 0.05, 1)
        elif common > 2 * min_common:  # Really hope it does not happen
            common = 2 * min_common
        assert common < 1  # Very brittle configuration here

        self.common_g = common
        self.common_b = common
        parents = 1 / (1 - min_common / common)
        assert parents > 2  # This is brittle as well
        self.parents_g = parents
        self.parents_b = parents


class GeneratorFaultTree(FaultTree):
    """Specialization of a fault tree for generation purposes.

    The construction of fault tree members are handled through this object.
    It is assumed that no removal is going to happen after construction.

    Args:
        factors: The fault tree generation factors.
    """

    def __init__(self, name, factors):
        """Initializes an empty fault tree.

        Args:
            name: The name of the system described by the fault tree container.
            factors: Fully configured generation factors.
        """
        super(GeneratorFaultTree, self).__init__(name)
        self.factors = factors

    def construct_top_gate(self, root_name):
        """Constructs and assigns a new gate suitable for being a root.

        Args:
            root_name: Unique name for the root gate.
        """
        assert not self.top_gate and not self.top_gates
        operator = self.factors.get_random_operator()
        while operator == "xor" or operator == "not":
            operator = self.factors.get_random_operator()
        self.top_gate = Gate(root_name, operator)
        self.gates.append(self.top_gate)

    def construct_gate(self):
        """Constructs a new gate.

        Returns:
            A fully initialized gate with random attributes.
        """
        gate = Gate("G" + str(len(self.gates) + 1),
                    self.factors.get_random_operator())
        self.gates.append(gate)
        return gate

    def construct_basic_event(self):
        """Constructs a basic event with a unique identifier.

        Returns:
            A fully initialized basic event with a random probability.
        """
        basic_event = BasicEvent("B" + str(len(self.basic_events) + 1),
                                 random.uniform(self.factors.min_prob,
                                                self.factors.max_prob))
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
        ccf_group.prob = random.uniform(self.factors.min_prob,
                                        self.factors.max_prob)
        ccf_group.model = "MGL"
        levels = random.randint(2, len(members))
        ccf_group.factors = [random.uniform(0.1, 1) for _ in range(levels - 1)]
        return ccf_group


def candidate_gates(common_gate):
    """Lazy generator of candidates for common gates.

    Args:
        common_gate: A list of common gates.

    Yields:
        A next gate candidate from common gates container.
    """
    orphans = [x for x in common_gate if not x.parents]
    random.shuffle(orphans)
    for i in orphans:
        yield i

    single_parent = [x for x in common_gate if len(x.parents) == 1]
    random.shuffle(single_parent)
    for i in single_parent:
        yield i

    multi_parent = [x for x in common_gate if len(x.parents) > 1]
    random.shuffle(multi_parent)
    for i in multi_parent:
        yield i


def correct_for_exhaustion(gates_queue, common_gate, fault_tree):
    """Corrects the generation for queue exhaustion.

    Corner case when not enough new basic events initialized,
    but there are no more intermediate gates to use
    due to a big ratio or just random accident.

    Args:
        gates_queue: A deque of gates to be initialized.
        common_gate: A list of common gates.
        fault_tree: The fault tree container of all events and constructs.
    """
    if gates_queue:
        return
    if len(fault_tree.basic_events) < fault_tree.factors.num_basic:
        # Initialize one more gate
        # by randomly choosing places in the fault tree.
        random_gate = random.choice(fault_tree.gates)
        while (random_gate.operator == "not" or
               random_gate.operator == "xor" or
               random_gate in common_gate):
            random_gate = random.choice(fault_tree.gates)
        new_gate = fault_tree.construct_gate()
        random_gate.add_argument(new_gate)
        gates_queue.append(new_gate)


def choose_basic_event(s_common, common_basic, fault_tree):
    """Creates a new basic event or uses a common one for gate arguments.

    Args:
        s_common: Sampled factor to choose common basic events.
        common_basic: A list of common basic events to choose from.
        fault_tree: The fault tree container of all events and constructs.

    Returns:
        Basic event argument for a gate.
    """
    if s_common < fault_tree.factors.common_b and common_basic:
        orphans = [x for x in common_basic if not x.parents]
        if orphans:
            return random.choice(orphans)

        single_parent = [x for x in common_basic if len(x.parents) == 1]
        if single_parent:
            return random.choice(single_parent)

        return random.choice(common_basic)
    else:
        return fault_tree.construct_basic_event()


def init_gates(gates_queue, common_basic, common_gate, fault_tree):
    """Initializes gates and other basic events.

    Args:
        gates_queue: A deque of gates to be initialized.
        common_basic: A list of common basic events.
        common_gate: A list of common gates.
        fault_tree: The fault tree container of all events and constructs.
    """
    # Get an intermediate gate to initialize breadth-first
    gate = gates_queue.popleft()

    num_arguments = fault_tree.factors.get_num_args(gate)

    ancestors = None  # needed for cycle prevention
    max_tries = len(common_gate)  # the number of maximum tries
    num_tries = 0  # the number of tries to get a common gate

    # pylint: disable=too-many-nested-blocks
    # This code is both hot and coupled for performance reasons.
    # There may be a better solution than the current approach.
    while gate.num_arguments() < num_arguments:
        s_percent = random.random()  # sample percentage of gates
        s_common = random.random()  # sample the reuse frequency

        # Case when the number of basic events is already satisfied
        if len(fault_tree.basic_events) == fault_tree.factors.num_basic:
            s_common = 0  # use only common nodes

        if s_percent < fault_tree.factors.get_percent_gate():
            # Create a new gate or use a common one
            if s_common < fault_tree.factors.common_g and num_tries < max_tries:
                # Lazy evaluation of ancestors
                if not ancestors:
                    ancestors = gate.get_ancestors()

                for random_gate in candidate_gates(common_gate):
                    num_tries += 1
                    if num_tries >= max_tries:
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
            gate.add_argument(choose_basic_event(s_common, common_basic,
                                                 fault_tree))

    correct_for_exhaustion(gates_queue, common_gate, fault_tree)


def distribute_house_events(fault_tree):
    """Distributes house events to already initialized gates.

    Args:
        fault_tree: The fault tree container of all events and constructs.
    """
    while len(fault_tree.house_events) < fault_tree.factors.num_house:
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
    if fault_tree.factors.num_ccf:
        members = fault_tree.basic_events[:]
        random.shuffle(members)
        first_mem = 0
        last_mem = 0
        while len(fault_tree.ccf_groups) < fault_tree.factors.num_ccf:
            max_args = int(2 * fault_tree.factors.num_args - 2)
            group_size = random.randint(2, max_args)
            last_mem = first_mem + group_size
            if last_mem > len(members):
                break
            fault_tree.construct_ccf_group(members[first_mem:last_mem])
            first_mem = last_mem
        fault_tree.non_ccf_events = members[first_mem:]


def generate_fault_tree(ft_name, root_name, factors):
    """Generates a fault tree of specified complexity.

    The Factors class attributes are used as parameters for complexity.

    Args:
        ft_name: The name of the fault tree.
        root_name: The name for the root gate of the fault tree.
        factors: Factors for fault tree generation.

    Returns:
        Top gate of the created fault tree.
    """
    fault_tree = GeneratorFaultTree(ft_name, factors)
    fault_tree.construct_top_gate(root_name)

    # Estimating the parameters
    num_gate = factors.get_num_gate()
    num_common_basic = factors.get_num_common_basic(num_gate)
    num_common_gate = factors.get_num_common_gate(num_gate)
    common_basic = [fault_tree.construct_basic_event()
                    for _ in range(num_common_basic)]
    common_gate = [fault_tree.construct_gate()
                   for _ in range(num_common_gate)]

    # Container for not yet initialized gates
    # A deque is used to traverse the tree breadth-first
    gates_queue = deque()
    gates_queue.append(fault_tree.top_gate)
    while gates_queue:
        init_gates(gates_queue, common_basic, common_gate, fault_tree)

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
    factors = fault_tree.factors
    tree_file.write("<?xml version=\"1.0\"?>\n")
    tree_file.write(
        "<!--\nThis is a description of the auto-generated fault tree\n"
        "with the following parameters:\n\n"
        "The output file name: " + tree_file.name + "\n"
        "The fault tree name: " + fault_tree.name + "\n"
        "The root gate name: " + fault_tree.top_gate.name + "\n\n"
        "The seed of the random number generator: " + str(seed) + "\n"
        "The number of basic events: " + str(factors.num_basic) + "\n"
        "The number of house events: " + str(factors.num_house) + "\n"
        "The number of CCF groups: " + str(factors.num_ccf) + "\n"
        "The average number of gate arguments: " +
        str(factors.num_args) + "\n"
        "The weights of gate types [AND, OR, K/N, NOT, XOR]: " +
        str(factors.get_gate_weights()) + "\n"
        "Percentage of common basic events per gate: " +
        str(factors.common_b) + "\n"
        "Percentage of common gates per gate: " +
        str(factors.common_g) + "\n"
        "The avg. number of parents for common basic events: " +
        str(factors.parents_b) + "\n"
        "The avg. number of parents for common gates: " +
        str(factors.parents_g) + "\n"
        "Maximum probability for basic events: " +
        str(factors.max_prob) + "\n"
        "Minimum probability for basic events: " +
        str(factors.min_prob) + "\n"
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


def manage_cmd_args(argv=None):
    """Manages command-line description and arguments.

    Args:
        argv: An optional list containing the command-line arguments.
            If None, the command-line arguments from sys will be used.

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
    parser.add_argument("--aralia", action="store_true",
                        help="apply the Aralia format to the output")
    parser.add_argument("--nest", type=int, default=0, metavar="int",
                        help="nestedness of Boolean formulae in the XML output")
    args = parser.parse_args(argv)
    if args.nest < 0:
        raise ap.ArgumentTypeError("The nesting factor cannot be negative")
    if args.aralia:
        if args.out == "fault_tree.xml":
            args.out = "fault_tree.txt"
    return args


def setup_factors(args):
    """Configures the fault generation by assigning factors.

    Args:
        args: Command-line arguments with values for factors.

    Returns:
        Fully initialized Factors object.

    Raises:
        ArgumentTypeError: Problems with the arguments.
        FactorError: Invalid setup for factors.
    """
    random.seed(args.seed)
    factors = Factors()
    factors.set_min_max_prob(args.min_prob, args.max_prob)
    factors.set_common_event_factors(args.common_b, args.common_g,
                                     args.parents_b, args.parents_g)
    factors.set_num_factors(args.num_args, args.num_basic, args.num_house,
                            args.num_ccf)
    factors.set_gate_weights([float(i) for i in args.weights_g])
    if args.num_gate:
        factors.constrain_num_gate(args.num_gate)
    factors.calculate()
    return factors


def main(argv=None):
    """The main function of the fault tree generator.

    Args:
        argv: An optional list containing the command-line arguments.
            If None, the command-line arguments from sys will be used.

    Raises:
        ArgumentTypeError: There are problems with the arguments.
        FactorError: Invalid setup for factors.
    """
    args = manage_cmd_args(argv)
    factors = setup_factors(args)
    fault_tree = generate_fault_tree(args.ft_name, args.root, factors)
    with open(args.out, "w") as tree_file:
        if args.aralia:
            tree_file.write(fault_tree.to_aralia())
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
