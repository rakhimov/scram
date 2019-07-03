# Copyright (C) 2014-2018 Olzhas Rakhimov
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

import random
from subprocess import call
from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
import pytest

from fault_tree_generator import FactorError, Factors, generate_fault_tree, \
    write_info, write_summary, main

# pylint: disable=redefined-outer-name


@pytest.fixture()
def factors():
    """Creates partially constructed factors collection."""
    return Factors()


def test_min_max_prob_valid(factors):
    """Tests setting of valid probability min-max factors."""
    factors.set_min_max_prob(0.1, 0.5)
    assert factors.min_prob == 0.1
    assert factors.max_prob == 0.5


@pytest.mark.parametrize("min_value,max_value", [(-0.1, 0.5), (1.1, 0.5),
                                                 (0.1, -0.5), (0.1, 1.5),
                                                 (0.5, 0.1)])
def test_min_max_prob_fail(factors, min_value, max_value):
    """Tests setting of invalid probability min-max factors."""
    with pytest.raises(FactorError):
        factors.set_min_max_prob(min_value, max_value)


def test_set_common_event_valid(factors):
    """Tests valid common event factors."""
    factors.set_common_event_factors(0.4, 0.2, 3, 4)
    assert factors.common_b == 0.4
    assert factors.common_g == 0.2
    assert factors.parents_b == 3
    assert factors.parents_g == 4


@pytest.mark.parametrize("args", [(-0.1, 0.5, 2, 2), (1.0, 0.5, 2, 2),
                                  (0.1, -0.5, 2, 2), (0.1, 1.0, 2, 2),
                                  (0, 0, 2, 2), (0.1, 0.1, 1, 2),
                                  (0.1, 0.1, 101, 2), (0.1, 0.1, 2, 1),
                                  (0.1, 0.1, 2, 101)])
def test_set_common_event_fail(factors, args):
    """Tests setting of invalid common event factors."""
    factors.set_common_event_factors(0.1, 0.1, 2, 2)  # no fail
    with pytest.raises(FactorError):
        factors.set_common_event_factors(*args)


def test_set_num_factors_valid(factors):
    """Tests setting of valid size factors."""
    factors.set_num_factors(3, 100, 5, 4)
    assert factors.num_args == 3
    assert factors.num_basic == 100
    assert factors.num_house == 5
    assert factors.num_ccf == 4


@pytest.mark.parametrize(
    "args",
    [
        (1.5, 100),
        (3, 0),
        (3, 100, -5),
        (3, 100, 0, -4),
        # Too many house events.
        (3, 5, 5),
        # Too many CCF groups.
        (5, 50, 0, 11)
    ])
def test_set_num_factors_fail(factors, args):
    """Tests setting of invalid size factors."""
    with pytest.raises(FactorError):
        factors.set_num_factors(*args)


@pytest.mark.parametrize(
    "args,expected",
    [
        ([5, 8, 4, 2, 1], [5, 8, 4, 2, 1]),
        ([5, 8, 4], [5, 8, 4, 0, 0])  # for padding with 0s
    ])
def test_set_gate_weights_valid(factors, args, expected):
    """Tests the setting of valid gate weights."""
    factors.set_gate_weights(args)
    assert factors.get_gate_weights() == expected


@pytest.mark.parametrize(
    "args",
    [
        [],
        [-1, 2, 3],
        [0, 0, 0],
        # Too many weights.
        [1, 2, 3, 4, 5, 6],
        # XOR or NOT only.
        [0, 0, 0, 1, 2],
    ])
def test_set_gate_weights_fail(factors, args):
    """Tests the setting of invalid gate weights."""
    with pytest.raises(FactorError):
        factors.set_gate_weights(args)


def test_constrain_num_gates(factors):
    """Checks invalid setup for constraining gate numbers."""
    with pytest.raises(FactorError):
        factors.constrain_num_gate(0)
    factors.num_args = 4
    factors.num_basic = 400
    with pytest.raises(FactorError):
        factors.constrain_num_gate(50)  # unsatisfiable


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
        assert fault_tree is not None

        def printer(*args):
            """Temporary printer."""
            print(*args, sep='', file=self.output)

        write_info(fault_tree, printer, 123)
        write_summary(fault_tree, printer)
        fault_tree.to_xml(printer, True)
        self.output.flush()
        relaxng_doc = etree.parse("../share/input.rng")
        relaxng = etree.RelaxNG(relaxng_doc)
        with open(self.output.name, "r") as test_file:
            doc = etree.parse(test_file)
            assert relaxng.validate(doc)

    def test_aralia_output(self):
        """Checks if the Aralia format output passes validation."""
        self.factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        self.factors.num_house = 10
        self.factors.num_ccf = 10
        self.factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root", self.factors)
        assert fault_tree is not None
        fault_tree.to_aralia(lambda *args: print(
            *args, sep='', file=self.output))
        self.output.file.flush()
        tmp = NamedTemporaryFile(mode="w+")
        cmd = ["./translators/aralia.py", self.output.name, "-o", tmp.name]
        assert call(cmd) == 0

    def test_constrain_num_gates(self):
        """Checks the case of the constrained number of gates."""
        self.factors.set_gate_weights([1, 1, 1, 0.1, 0.1])
        self.factors.num_basic = 200
        self.factors.constrain_num_gate(200)
        self.factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root", self.factors)
        assert fault_tree is not None
        assert abs(1 - len(fault_tree.gates) / 200) < 0.1


def test_main():
    """Tests the main() of the generator."""
    tmp = NamedTemporaryFile(mode="w+")
    main(["-b", "200", "-g", "200", "-o", tmp.name])
    relaxng_doc = etree.parse("../share/input.rng")
    relaxng = etree.RelaxNG(relaxng_doc)
    with open(tmp.name, "r") as test_file:
        doc = etree.parse(test_file)
        assert relaxng.validate(doc)

    main(["-b", "200", "-g", "200", "-o", tmp.name, "--aralia"])
    cmd = [
        "./translators/aralia.py", tmp.name, "-o",
        NamedTemporaryFile(mode="w+").name
    ]
    assert call(cmd) == 0
