#!/usr/bin/env python
"""test_shorthand_to_xml.py

Tests for the shorthand-to-XML converter.
"""

from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
from nose.tools import assert_raises, assert_is_not_none, assert_equal, \
        assert_true

from shorthand_to_xml import *

def test_correct():
    """Tests the valid overall process."""
    tmp = NamedTemporaryFile()
    tmp.write("ValidFaultTree\n\n")
    tmp.write("root := g1 | g2 | g3 | g4 | e1\n")
    tmp.write("g1 := e2 | g3 & g5\n")
    tmp.write("g2 := h1 & g6\n")
    tmp.write("g3 := (g6 ^ e2)\n")
    tmp.write("g4 := @(2, [g5, e3, e4])\n")
    tmp.write("g5 := ~e3\n")
    tmp.write("g6 := ((e3 | e4))\n\n")
    tmp.write("p(e1) = 0.1\n")
    tmp.write("p(e2) = 0.2\n")
    tmp.write("p(e3) = 0.3\n")
    tmp.write("s(h1) = true\n")
    tmp.write("s(h2) = false\n")
    tmp.flush()
    fault_tree = parse_input_file(tmp.name)
    assert_is_not_none(fault_tree)
    out = NamedTemporaryFile()
    write_to_xml_file(fault_tree, out.name)
    relaxng_doc = etree.parse("../share/input.rng")
    relaxng = etree.RelaxNG(relaxng_doc)
    doc = etree.parse(out)
    assert_true(relaxng.validate(doc))

def test_ft_name_redefinition():
    """Tests the redefinition of the fault tree name."""
    tmp = NamedTemporaryFile()
    tmp.write("FaultTreeName\n")
    tmp.write("AnotherFaultTree\n")
    tmp.flush()
    assert_raises(FormatError, parse_input_file, tmp.name)

def test_ncname_ft():
    """The name of the fault tree must conform to NCNAME format."""
    tmp = NamedTemporaryFile()
    tmp.write("NOT NCNAME Fault tree.\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("NOT-NCNAME-Fault-tree.\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name

def test_no_ft_name():
    """Tests the case where no fault tree name is provided."""
    tmp = NamedTemporaryFile()
    tmp.write("g1 := g2 & e1\n")
    tmp.write("g2 := h1 & e1\n")
    tmp.flush()
    assert_raises(FormatError, parse_input_file, tmp.name)

def test_illegal_format():
    """Test Arithmetic operators."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 + e1\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 * e1\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := -e1\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 / e1\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name

def test_repeated_argument():
    """Tests the formula with a repeated argument."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 & e1\n")
    tmp.write("g2 := E1 & e1\n")  # repeated argument with uppercase
    tmp.flush()
    assert_raises(FaultTreeError, parse_input_file, tmp.name)

def test_parenthesis_count():
    """Tests cases with missing parenthesis."""
    tmp = NamedTemporaryFile()
    tmp.write("WrongParentheses\n")
    tmp.write("g1 := (a | b & c")  # missing the closing parenthesis
    tmp.flush()
    assert_raises(FormatError, parse_input_file, tmp.name)

def test_combination_gate_children():
    """K/N or Combination gate/operator should have its K < its N."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := @(3, [a, b, c])")  # K = N
    tmp.flush()
    yield assert_raises, FaultTreeError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := @(4, [a, b, c])")  # K > N
    tmp.flush()
    yield assert_raises, FaultTreeError, parse_input_file, tmp.name

def test_no_top_event():
    """Detection of cases without top gate definitions.

    Note that this also means that there is a cycle that includes the root.
    """
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 & e1\n")
    tmp.write("g2 := g1 & e1\n")
    tmp.flush()
    assert_raises(FaultTreeError, parse_input_file, tmp.name)

def test_multi_top():
    """Multiple root nodes without the flag causes a problem by default."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := e2 & e1\n")
    tmp.write("g2 := h1 & e1\n")
    tmp.flush()
    yield assert_raises, FaultTreeError, parse_input_file, tmp.name
    yield assert_is_not_none, parse_input_file(tmp.name, True)  # with the flag

def test_redefinition():
    """Tests name collision detection of nodes."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 & e1\n")
    tmp.write("g2 := h1 & e1\n")
    tmp.write("g2 := e2 & e1\n")  # redefining a node
    tmp.flush()
    assert_raises(FaultTreeError, parse_input_file, tmp.name)

def test_orphan_nodes():
    """Tests cases with orphan house and basic event nodes."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 & e1\n")
    tmp.write("g2 := h1 & e1\n")
    tmp.write("p(e1) = 0.5\n")
    tmp.write("s(h1) = false\n")
    tmp.flush()
    yield assert_is_not_none, parse_input_file(tmp.name)
    tmp.write("p(e2) = 0.1\n")  # orphan basic event
    tmp.flush()
    yield assert_is_not_none, parse_input_file(tmp.name)
    tmp.write("s(h2) = true\n")  # orphan house event
    tmp.flush()
    yield assert_is_not_none, parse_input_file(tmp.name)

def test_cycle_detection():
    """Tests cycles in the fault tree"""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := g2 & e1\n")
    tmp.write("g2 := g3 & e1\n")
    tmp.write("g3 := g2 & e1\n")  # cycle
    tmp.flush()
    yield assert_raises, FaultTreeError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := u1 | g2 & e1\n")
    tmp.write("g2 := u2 | g3 & e1\n")  # nested formula cycle
    tmp.write("g3 := u3 | g2 & e1\n")  # cycle
    tmp.flush()
    yield assert_raises, FaultTreeError, parse_input_file, tmp.name

def test_detached_gates():
    """Some cycles may get detached from the original fault tree."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := e2 & e1\n")
    tmp.write("g2 := g3 & e1\n")  # detached gate
    tmp.write("g3 := g2 & e1\n")  # cycle
    tmp.flush()
    assert_raises(FaultTreeError, parse_input_file, tmp.name)

class OperatorPrecedenceTestCase(TestCase):
    """Logical operator precedence tests."""

    def setUp(self):
        self.tmp = NamedTemporaryFile()
        self.tmp.write("FT\n")

    def two_operators(self, op_one, op_two, num_args=3):
        """Common operations for two operator tests.

        The requirement for the input is to have g1 gate with a nested formula
        with num_args arguments. Only the first argument is associated with
        the first operator, and the rest of arguments are associated with the
        second operator. The arguments must be named e1, e2, e3, ...

        Args:
            op_one: The first operator of higher precedence.
            op_two: The second operator of lower precedence.
            num_args: The number of arguments in the nested formula.
        """
        args = []
        for i in range(1, num_args + 1):
            args.append("e%d" % i)

        fault_tree = parse_input_file(self.tmp.name)
        assert_equal(len(fault_tree.gates), 1)
        gate = fault_tree.gates["g1"].formula
        assert_equal(gate.num_arguments(), 2)
        assert_equal(gate.operator, op_one)
        assert_equal(gate.node_arguments, args[:1])
        assert_equal(len(gate.f_arguments), 1)
        child_f = gate.f_arguments[0]
        assert_equal(child_f.num_arguments(), num_args - 1)
        assert_equal(child_f.operator, op_two)
        assert_equal(child_f.node_arguments, args[1:])

    def test_or_xor(self):
        """Formula with OR and XOR operators."""
        self.tmp.write("g1 := e1 | e2 ^ e3\n")
        self.tmp.flush()
        self.two_operators("or", "xor")

    def test_or_and(self):
        """Formula with OR and AND operators."""
        self.tmp.write("g1 := e1 | e2 & e3\n")
        self.tmp.flush()
        self.two_operators("or", "and")

    def test_or_atleast(self):
        """Formula with OR and K/N operators."""
        self.tmp.write("g1 := e1 | @(2, [e2, e3, e4])\n")
        self.tmp.flush()
        self.two_operators("or", "atleast", 4)

    def test_or_not(self):
        """Formula with OR and NOT operators."""
        self.tmp.write("g1 := e1 | ~e2\n")
        self.tmp.flush()
        self.two_operators("or", "not", 2)

    def test_xor_xor(self):
        """Formula with XOR and XOR operators.

        Note that this is a special case because most analysis restricts
        XOR operator to two arguments, so nested formula of XOR operators
        must be created.
        """
        self.tmp.write("g1 := e1 ^ e2 ^ e3\n")
        self.tmp.flush()
        self.two_operators("xor", "xor")

    def test_xor_and(self):
        """Formula with XOR and AND operators."""
        self.tmp.write("g1 := e1 ^ e2 & e3\n")
        self.tmp.flush()
        self.two_operators("xor", "and")

    def test_xor_atleast(self):
        """Formula with XOR and K/N operators."""
        self.tmp.write("g1 := e1 ^ @(2, [e2, e3, e4])\n")
        self.tmp.flush()
        self.two_operators("xor", "atleast", 4)

    def test_xor_not(self):
        """Formula with XOR and NOT operators."""
        self.tmp.write("g1 := e1 ^ ~e2\n")
        self.tmp.flush()
        self.two_operators("xor", "not", 2)

    def test_and_atleast(self):
        """Formula with AND and K/N operators."""
        self.tmp.write("g1 := e1 & @(2, [e2, e3, e4])\n")
        self.tmp.flush()
        self.two_operators("and", "atleast", 4)

    def test_and_not(self):
        """Formula with AND and NOT operators."""
        self.tmp.write("g1 := e1 & ~e2\n")
        self.tmp.flush()
        self.two_operators("and", "not", 2)

class ParenthesesTestCase(TestCase):
    """Application of parentheses tests."""

    def setUp(self):
        self.tmp = NamedTemporaryFile()
        self.tmp.write("FT\n")

    def test_atleast_not(self):
        """Formula with K/N containing NOT operator."""
        self.tmp.write("g1 := @(2, [~e1, e2, e3])\n")
        self.tmp.flush()

    def test_not_atleast(self):
        """Formula with negation of K/N operator."""
        self.tmp.write("g1 := ~@(2, [e1, e2, e3])\n")
        self.tmp.flush()
