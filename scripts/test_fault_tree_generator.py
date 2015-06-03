#!/usr/bin/env python
"""test_fault_tree_generator.py

Tests for the fault tree generator.
"""

from subprocess import call
from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
from nose.tools import assert_equal, assert_true, assert_is_not_none

from fault_tree_generator import Settings, Factors, generate_fault_tree, \
        write_results, write_shorthand, BasicEvent, Gate, HouseEvent, \
        CcfGroup

class FaultTreeGeneratorTestCase(TestCase):
    """General tests for the fault tree generator script."""
    def setUp(self):
        Gate.gates = []
        Gate.num_gates = 0
        BasicEvent.num_basic = 0
        BasicEvent.basic_events = []
        BasicEvent.non_ccf_events = []
        HouseEvent.num_house = 0
        HouseEvent.house_events = []
        CcfGroup.num_ccf = 0
        CcfGroup.ccf_groups = []
        self.output = NamedTemporaryFile()
        Settings.output = self.output.name
        Settings.ft_name = "TestingTree"
        Settings.root_name = "root"
        Factors.num_basics = 10000
        Factors.num_house = 0
        Factors.num_ccf = 0
        Factors.common_b = 0.1
        Factors.common_g = 0.1
        Factors.avg_children = 3
        Factors.parents_b = 2
        Factors.parents_g = 2
        Factors.set_weights([1, 1, 0, 0, 0])
        BasicEvent.min_prob = 0.01
        BasicEvent.max_prob = 0.1
        Factors.calculate()

    def test_xml_output(self):
        """Checks if the XML output passes the schema validation."""
        Factors.set_weights([1, 1, 1, 0.1, 0.1])
        Factors.num_house = 10
        Factors.num_ccf = 10
        Factors.calculate()
        Settings.nested = True
        top_node = generate_fault_tree()
        assert_is_not_none(top_node)
        write_results(top_node)
        relaxng_doc = etree.parse("../share/input.rng")
        relaxng = etree.RelaxNG(relaxng_doc)
        doc = etree.parse(self.output)
        assert_true(relaxng.validate(doc))

    def test_shorthand_output(self):
        """Checks if the shorthand format output passes validation."""
        Factors.set_weights([1, 1, 1, 0.1, 0.1])
        Factors.num_house = 10
        Factors.num_ccf = 10
        Factors.calculate()
        top_node = generate_fault_tree()
        assert_is_not_none(top_node)
        write_shorthand(top_node)
        tmp = NamedTemporaryFile()
        cmd = ["./shorthand_to_xml.py", self.output.name, "-o", tmp.name]
        assert_equal(0, call(cmd))
