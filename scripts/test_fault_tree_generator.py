#!/usr/bin/env python
"""test_fault_tree_generator.py

Tests for the fault tree generator.
"""

from subprocess import call
from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
from nose.tools import assert_raises, assert_is_not_none, assert_equal, \
        assert_true

from fault_tree_generator import *

def test_xml_output():
    """Checks if the XML output passes the schema validation."""
    out = NamedTemporaryFile()
    cmd = ["./fault_tree_generator.py", "-o", out.name]
    yield assert_equal, 0, call(cmd)
    relaxng_doc = etree.parse("../share/input.rng")
    relaxng = etree.RelaxNG(relaxng_doc)
    doc = etree.parse(out)
    yield assert_true, relaxng.validate(doc)

def test_shorthand_output():
    """Checks if the shorthand format output passes validation."""
    out = NamedTemporaryFile()
    cmd = ["./fault_tree_generator.py", "-o", out.name, "--shorthand"]
    assert_equal(0, call(cmd))
    tmp = NamedTemporaryFile()
    cmd = ["./shorthand_to_xml.py", out.name, "-o", tmp.name]
    assert_equal(0, call(cmd))
