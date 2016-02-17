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

"""Tests for the fault tree generator."""

from __future__ import division

import random
from subprocess import call
from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
from nose.tools import assert_equal, assert_true, assert_is_not_none, \
    assert_less, assert_raises

from fault_tree_generator import FactorError, Factors, generate_fault_tree, \
    write_info, write_summary, main


class FactorsTestCase(TestCase):
    """Tests for correct setting and calculation of factors."""

    def test_min_max_prob(self):
        """Tests setting of probability factors."""
        assert_raises(FactorError, Factors.set_min_max_prob, -0.1, 0.5)
        assert_raises(FactorError, Factors.set_min_max_prob, 1.1, 0.5)
        assert_raises(FactorError, Factors.set_min_max_prob, 0.1, -0.5)
        assert_raises(FactorError, Factors.set_min_max_prob, 0.1, 1.5)
        assert_raises(FactorError, Factors.set_min_max_prob, 0.5, 0.1)
        Factors.set_min_max_prob(0.1, 0.5)
        assert_equal(0.1, Factors.min_prob)
        assert_equal(0.5, Factors.max_prob)

    def test_set_common_event_factors(self):
        """Tests setting of probability factors."""
        Factors.set_common_event_factors(0.1, 0.1, 2, 2)  # no fail
        assert_raises(FactorError, Factors.set_common_event_factors,
                      -0.1, 0.5, 2, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      1.0, 0.5, 2, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0.1, -0.5, 2, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0.1, 1.0, 2, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0, 0, 2, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0.1, 0.1, 1, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0.1, 0.1, 101, 2)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0.1, 0.1, 2, 1)
        assert_raises(FactorError, Factors.set_common_event_factors,
                      0.1, 0.1, 2, 101)
        Factors.set_common_event_factors(0.4, 0.2, 3, 4)
        assert_equal(0.4, Factors.common_b)
        assert_equal(0.2, Factors.common_g)
        assert_equal(3, Factors.parents_b)
        assert_equal(4, Factors.parents_g)

    def test_set_num_factors(self):
        """Tests setting of size factors."""
        Factors.set_num_factors(3, 100, 5, 4)
        assert_equal(3, Factors.num_args)
        assert_equal(100, Factors.num_basic)
        assert_equal(5, Factors.num_house)
        assert_equal(4, Factors.num_ccf)
        # Invalid values.
        assert_raises(FactorError, Factors.set_num_factors, 1.5, 100)
        assert_raises(FactorError, Factors.set_num_factors, 3, 0)
        assert_raises(FactorError, Factors.set_num_factors, 3, 100, -5)
        assert_raises(FactorError, Factors.set_num_factors, 3, 100, 0, -4)
        # Too many house events.
        assert_raises(FactorError, Factors.set_num_factors, 3, 5, 5)
        # Too many CCF groups.
        assert_raises(FactorError, Factors.set_num_factors, 5, 50, 0, 11)

    def test_set_gate_weights(self):
        """Tests the setting of gate weights."""
        assert_raises(FactorError, Factors.set_gate_weights, [])
        assert_raises(FactorError, Factors.set_gate_weights, [-1, 2, 3])
        assert_raises(FactorError, Factors.set_gate_weights, [0, 0, 0])
        # Too many weights.
        assert_raises(FactorError, Factors.set_gate_weights, [1, 2, 3, 4, 5, 6])
        # XOR or NOT only.
        assert_raises(FactorError, Factors.set_gate_weights, [0, 0, 0, 1, 2])
        Factors.set_gate_weights([5, 8, 4, 2, 1])
        assert_equal([5, 8, 4, 2, 1], Factors.get_gate_weights())
        Factors.set_gate_weights([5, 8, 4])  # for padding with 0s
        assert_equal([5, 8, 4, 0, 0], Factors.get_gate_weights())

    def test_constrain_num_gates(self):
        """Checks invalid setup for constraining gate numbers."""
        assert_raises(FactorError, Factors.constrain_num_gate, 0)
        Factors.num_args = 4
        Factors.num_basic = 400
        assert_raises(FactorError, Factors.constrain_num_gate, 50)  # not enough


class FaultTreeGeneratorTestCase(TestCase):
    """General tests for the fault tree generator script."""

    def setUp(self):
        """Initializes the generator factors for default complexity."""
        self.output = NamedTemporaryFile()
        random.seed(123)
        Factors.num_basic = 10000
        Factors.num_house = 0
        Factors.num_ccf = 0
        Factors.common_b = 0.1
        Factors.common_g = 0.1
        Factors.num_args = 3
        Factors.parents_b = 2
        Factors.parents_g = 2
        Factors.set_gate_weights([1, 1, 0, 0, 0])
        Factors.min_prob = 0.01
        Factors.max_prob = 0.1
        Factors.calculate()

    def test_xml_output(self):
        """Checks if the XML output passes the schema validation."""
        Factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        Factors.num_house = 10
        Factors.num_ccf = 10
        Factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root")
        assert_is_not_none(fault_tree)
        write_info(fault_tree, self.output, 123)
        write_summary(fault_tree, self.output)
        self.output.write(fault_tree.to_xml(1))
        self.output.flush()
        relaxng_doc = etree.parse("../share/input.rng")
        relaxng = etree.RelaxNG(relaxng_doc)
        with open(self.output.name, "r") as test_file:
            doc = etree.parse(test_file)
            assert_true(relaxng.validate(doc))

    def test_shorthand_output(self):
        """Checks if the shorthand format output passes validation."""
        Factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        Factors.num_house = 10
        Factors.num_ccf = 10
        Factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root")
        assert_is_not_none(fault_tree)
        self.output.write(fault_tree.to_shorthand())
        self.output.file.flush()
        tmp = NamedTemporaryFile()
        cmd = ["./shorthand_to_xml.py", self.output.name, "-o", tmp.name]
        assert_equal(0, call(cmd))

    def test_constrain_num_gates(self):
        """Checks the case of the constrained number of gates."""
        Factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        Factors.num_basic = 200
        Factors.constrain_num_gate(200)
        Factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root")
        assert_is_not_none(fault_tree)
        assert_less(abs(1 - len(fault_tree.gates) / 200), 0.1)


def test_main():
    """Tests the main() of the generator."""
    tmp = NamedTemporaryFile()
    main(["-b", "200", "-g", "200", "-o", tmp.name])
    relaxng_doc = etree.parse("../share/input.rng")
    relaxng = etree.RelaxNG(relaxng_doc)
    with open(tmp.name, "r") as test_file:
        doc = etree.parse(test_file)
        assert_true(relaxng.validate(doc))

    main(["-b", "200", "-g", "200", "-o", tmp.name, "--shorthand"])
    cmd = ["./shorthand_to_xml.py", tmp.name, "-o", NamedTemporaryFile().name]
    assert_equal(0, call(cmd))
