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

"""Tests for the fault tree generator."""

from __future__ import division

import random
from subprocess import call
from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
from nose.tools import assert_equal, assert_true, assert_is_not_none, \
    assert_less

from fault_tree_generator import Factors, generate_fault_tree, write_info, \
    write_summary


class FaultTreeGeneratorTestCase(TestCase):
    """General tests for the fault tree generator script."""

    def setUp(self):
        """Initializes the generator factors for default complexity."""
        self.output = NamedTemporaryFile()
        random.seed(123)
        Factors.num_basics = 10000
        Factors.num_house = 0
        Factors.num_ccf = 0
        Factors.common_b = 0.1
        Factors.common_g = 0.1
        Factors.avg_children = 3
        Factors.parents_b = 2
        Factors.parents_g = 2
        Factors.set_weights([1, 1, 0, 0, 0])
        Factors.min_prob = 0.01
        Factors.max_prob = 0.1
        Factors.calculate()

    def test_xml_output(self):
        """Checks if the XML output passes the schema validation."""
        Factors.set_weights([1, 1, 1, 0.1, 0.1])
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
        Factors.set_weights([1, 1, 1, 0.1, 0.1])
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

    def test_constrained_num_gates(self):
        """Checks the case of the constrained number of gates."""
        Factors.set_weights([1, 1, 1, 0.1, 0.1])
        Factors.num_basics = 200
        Factors.constrain_num_gates(200)
        Factors.calculate()
        fault_tree = generate_fault_tree("TestingTree", "root")
        assert_is_not_none(fault_tree)
        assert_less(abs(1 - len(fault_tree.gates) / 200), 0.1)
