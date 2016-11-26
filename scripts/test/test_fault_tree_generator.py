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

from __future__ import division, absolute_import

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

    def setUp(self):
        """Creates partially constructed factors collection."""
        self.factors = Factors()

    def test_min_max_prob(self):
        """Tests setting of probability factors."""
        assert_raises(FactorError, self.factors.set_min_max_prob, -0.1, 0.5)
        assert_raises(FactorError, self.factors.set_min_max_prob, 1.1, 0.5)
        assert_raises(FactorError, self.factors.set_min_max_prob, 0.1, -0.5)
        assert_raises(FactorError, self.factors.set_min_max_prob, 0.1, 1.5)
        assert_raises(FactorError, self.factors.set_min_max_prob, 0.5, 0.1)
        self.factors.set_min_max_prob(0.1, 0.5)
        assert_equal(0.1, self.factors.min_prob)
        assert_equal(0.5, self.factors.max_prob)

    def test_set_common_event_factors(self):
        """Tests setting of probability factors."""
        self.factors.set_common_event_factors(0.1, 0.1, 2, 2)  # no fail
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      -0.1, 0.5, 2, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      1.0, 0.5, 2, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0.1, -0.5, 2, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0.1, 1.0, 2, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0, 0, 2, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0.1, 0.1, 1, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0.1, 0.1, 101, 2)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0.1, 0.1, 2, 1)
        assert_raises(FactorError, self.factors.set_common_event_factors,
                      0.1, 0.1, 2, 101)
        self.factors.set_common_event_factors(0.4, 0.2, 3, 4)
        assert_equal(0.4, self.factors.common_b)
        assert_equal(0.2, self.factors.common_g)
        assert_equal(3, self.factors.parents_b)
        assert_equal(4, self.factors.parents_g)

    def test_set_num_factors(self):
        """Tests setting of size factors."""
        self.factors.set_num_factors(3, 100, 5, 4)
        assert_equal(3, self.factors.num_args)
        assert_equal(100, self.factors.num_basic)
        assert_equal(5, self.factors.num_house)
        assert_equal(4, self.factors.num_ccf)
        # Invalid values.
        assert_raises(FactorError, self.factors.set_num_factors, 1.5, 100)
        assert_raises(FactorError, self.factors.set_num_factors, 3, 0)
        assert_raises(FactorError, self.factors.set_num_factors, 3, 100, -5)
        assert_raises(FactorError, self.factors.set_num_factors, 3, 100, 0, -4)
        # Too many house events.
        assert_raises(FactorError, self.factors.set_num_factors, 3, 5, 5)
        # Too many CCF groups.
        assert_raises(FactorError, self.factors.set_num_factors, 5, 50, 0, 11)

    def test_set_gate_weights(self):
        """Tests the setting of gate weights."""
        assert_raises(FactorError, self.factors.set_gate_weights, [])
        assert_raises(FactorError, self.factors.set_gate_weights, [-1, 2, 3])
        assert_raises(FactorError, self.factors.set_gate_weights, [0, 0, 0])
        # Too many weights.
        assert_raises(FactorError, self.factors.set_gate_weights,
                      [1, 2, 3, 4, 5, 6])
        # XOR or NOT only.
        assert_raises(FactorError, self.factors.set_gate_weights,
                      [0, 0, 0, 1, 2])
        self.factors.set_gate_weights([5, 8, 4, 2, 1])
        assert_equal([5, 8, 4, 2, 1], self.factors.get_gate_weights())
        self.factors.set_gate_weights([5, 8, 4])  # for padding with 0s
        assert_equal([5, 8, 4, 0, 0], self.factors.get_gate_weights())

    def test_constrain_num_gates(self):
        """Checks invalid setup for constraining gate numbers."""
        assert_raises(FactorError, self.factors.constrain_num_gate, 0)
        self.factors.num_args = 4
        self.factors.num_basic = 400
        assert_raises(FactorError, self.factors.constrain_num_gate, 50)


class FaultTreeGeneratorTestCase(TestCase):
    """General tests for the fault tree generator script."""

    def setUp(self):
        """Initializes the generator factors for default complexity."""
        self.output = NamedTemporaryFile(mode="w+")
        random.seed(123)
        self.factors = Factors()
        self.factors.set_min_max_prob(0.01, 0.1)
        self.factors.set_common_event_factors(0.1, 0.1, 2, 2)
        self.factors.set_num_factors(3, 10000)
        self.factors.set_gate_weights([1, 1, 0, 0, 0])
        self.factors.calculate()

    def test_xml_output(self):
        """Checks if the XML output passes the schema validation."""
        self.factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        self.factors.num_house = 10
        self.factors.num_ccf = 10
        self.factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root", self.factors)
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

    def test_aralia_output(self):
        """Checks if the Aralia format output passes validation."""
        self.factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        self.factors.num_house = 10
        self.factors.num_ccf = 10
        self.factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root", self.factors)
        assert_is_not_none(fault_tree)
        self.output.write(fault_tree.to_aralia())
        self.output.file.flush()
        tmp = NamedTemporaryFile(mode="w+")
        cmd = ["./translators/aralia.py", self.output.name, "-o", tmp.name]
        assert_equal(0, call(cmd))

    def test_constrain_num_gates(self):
        """Checks the case of the constrained number of gates."""
        self.factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        self.factors.num_basic = 200
        self.factors.constrain_num_gate(200)
        self.factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root", self.factors)
        assert_is_not_none(fault_tree)
        assert_less(abs(1 - len(fault_tree.gates) / 200), 0.1)


def test_main():
    """Tests the main() of the generator."""
    tmp = NamedTemporaryFile(mode="w+")
    main(["-b", "200", "-g", "200", "-o", tmp.name])
    relaxng_doc = etree.parse("../share/input.rng")
    relaxng = etree.RelaxNG(relaxng_doc)
    with open(tmp.name, "r") as test_file:
        doc = etree.parse(test_file)
        assert_true(relaxng.validate(doc))

    main(["-b", "200", "-g", "200", "-o", tmp.name, "--aralia"])
    cmd = ["./translators/aralia.py", tmp.name, "-o",
           NamedTemporaryFile(mode="w+").name]
    assert_equal(0, call(cmd))
