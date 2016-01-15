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

"""Tests for the shorthand-to-XML converter."""

from tempfile import NamedTemporaryFile
from unittest import TestCase

from lxml import etree
from nose.tools import assert_raises, assert_is_not_none, assert_equal, \
    assert_true

from shorthand_to_xml import ParsingError, FormatError, FaultTreeError, \
    parse_input_file, write_to_xml_file


def test_correct():
    """Tests the valid overall process."""
    tmp = NamedTemporaryFile()
    tmp.write("ValidFaultTree\n\n")
    tmp.write("root := g1 | g2 | g3 | g4 | g7 | e1\n")
    tmp.write("g1 := e2 | g3 & g5\n")
    tmp.write("g2 := h1 & g6\n")
    tmp.write("g3 := (g6 ^ e2)\n")
    tmp.write("g4 := @(2, [g5, e3, e4])\n")
    tmp.write("g5 := ~e3\n")
    tmp.write("g6 := ((e3 | e4))\n\n")
    tmp.write("g7 := e3\n\n")
    tmp.write("p(e1) = 0.1\n")
    tmp.write("p(e2) = 0.2\n")
    tmp.write("p(e3) = 0.3\n")
    tmp.write("s(h1) = true\n")
    tmp.write("s(h2) = false\n")
    tmp.flush()
    fault_tree = parse_input_file(tmp.name)
    assert_is_not_none(fault_tree)
    yield assert_equal, 8, len(fault_tree.gates)
    yield assert_equal, 3, len(fault_tree.basic_events)
    yield assert_equal, 2, len(fault_tree.house_events)
    yield assert_equal, 1, len(fault_tree.undef_nodes)
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
    tmp.write("Contains Whitespace Characters\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("Peri.od\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("EndWithDash-\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("Double--Dash\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("42StartWithNumbers\n")
    tmp.flush()
    yield assert_raises, ParsingError, parse_input_file, tmp.name
    tmp = NamedTemporaryFile()
    tmp.write("Correct-Name_42\n")
    tmp.write("g1 := e1 & e2\n")  # dummy gate
    tmp.flush()
    yield parse_input_file, tmp.name


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
    yield assert_raises, FormatError, parse_input_file, tmp.name
    # Other errors: (a|)b&c; ()a|b&c; a(|b&c); )a|b(&c;


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


def test_null_gate():
    """Tests if NULL type gates are recognized correctly."""
    tmp = NamedTemporaryFile()
    tmp.write("FT\n")
    tmp.write("g1 := a")
    tmp.flush()
    fault_tree = parse_input_file(tmp.name)
    assert_is_not_none(fault_tree)
    yield assert_equal, 1, len(fault_tree.gates)
    yield assert_true, "g1" in fault_tree.gates
    yield assert_true, "a" in fault_tree.gates["g1"].formula.node_arguments
    yield assert_equal, "null", fault_tree.gates["g1"].formula.operator


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
    """Tests cycles in the fault tree."""
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

        The requirement for the input is to have g1 gate
        with a nested formula
        with num_args arguments.
        Only the first argument is associated with the first operator,
        and the rest of arguments are associated with the second operator.
        The arguments must be named e1, e2, e3, ...

        Args:
            op_one: The first operator of higher precedence.
            op_two: The second operator of lower precedence.
            num_args: The number of arguments in the nested formula.
        """
        args = []
        for i in range(1, num_args + 1):
            args.append("e%d" % i)

        fault_tree = parse_input_file(self.tmp.name)
        assert_is_not_none(fault_tree)
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

        Note that this is a special case
        because most analysis restricts XOR operator to two arguments,
        so nested formula of XOR operators must be created.
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

    def test_not_not(self):
        """Formula with NOT and NOT operators.

        This is a special case. It is considered an error without parentheses.
        """
        self.tmp.write("g1 := ~~e1\n")
        self.tmp.flush()
        assert_raises(ParsingError, parse_input_file, self.tmp.name)
        tmp = NamedTemporaryFile()
        tmp.write("FT\n")
        tmp.write("g1 := ~e1~a\n")
        tmp.flush()
        assert_raises(ParsingError, parse_input_file, tmp.name)


class ParenthesesTestCase(TestCase):
    """Application of parentheses tests."""

    def setUp(self):
        self.tmp = NamedTemporaryFile()
        self.tmp.write("FT\n")

    def two_operators(self, op_one, op_two, num_args=3):
        """Common operations for two operator tests with parentheses.

        The requirement for the input is to have g1 gate
        with a nested formula
        with num_args arguments.
        Only the last argument is associated with the first operator,
        and the rest of arguments are associated with the second operator.
        The arguments must be named e1, e2, e3, ...

        Args:
            op_one: The first operator of higher precedence.
            op_two: The second operator of lower precedence.
            num_args: The number of arguments in the nested formula.
        """
        args = []
        for i in range(1, num_args + 1):
            args.append("e%d" % i)

        fault_tree = parse_input_file(self.tmp.name)
        assert_is_not_none(fault_tree)
        assert_equal(len(fault_tree.gates), 1)
        gate = fault_tree.gates["g1"].formula
        assert_equal(gate.num_arguments(), 2)
        assert_equal(gate.operator, op_one)
        assert_equal(gate.node_arguments, args[-1:])
        assert_equal(len(gate.f_arguments), 1)
        child_f = gate.f_arguments[0]
        assert_equal(child_f.num_arguments(), num_args - 1)
        assert_equal(child_f.operator, op_two)
        assert_equal(child_f.node_arguments, args[:-1])

    def test_or_xor(self):
        """Application of parentheses to OR and XOR operators."""
        self.tmp.write("g1 := (e1 | e2) ^ e3\n")
        self.tmp.flush()
        self.two_operators("xor", "or")

    def test_or_and(self):
        """Application of parentheses to OR and AND operators."""
        self.tmp.write("g1 := (e1 | e2) & e3\n")
        self.tmp.flush()
        self.two_operators("and", "or")

    def test_xor_and(self):
        """Application of parentheses to XOR and AND operators."""
        self.tmp.write("g1 := (e1 ^ e2) & e3\n")
        self.tmp.flush()
        self.two_operators("and", "xor")

    def negation(self, op_two, num_args=2):
        """Common operations with negation of formulas.

        The first operator must be NOT.

        Args:
            op_two: The second operator of lower precedence.
            num_args: The number of arguments in the nested formula.
        """
        args = []
        for i in range(1, num_args + 1):
            args.append("e%d" % i)
        fault_tree = parse_input_file(self.tmp.name)
        assert_is_not_none(fault_tree)
        assert_equal(len(fault_tree.gates), 1)
        gate = fault_tree.gates["g1"].formula
        assert_equal(gate.num_arguments(), 1)
        assert_equal(gate.operator, "not")
        assert_equal(gate.node_arguments, [])
        assert_equal(len(gate.f_arguments), 1)
        child_f = gate.f_arguments[0]
        assert_equal(child_f.num_arguments(), num_args)
        assert_equal(child_f.operator, op_two)
        assert_equal(child_f.node_arguments, args)

    def test_not_or(self):
        """Application of parentheses to OR and NOT operators."""
        self.tmp.write("g1 := ~(e1 | e2)\n")
        self.tmp.flush()
        self.negation("or")

    def test_not_xor(self):
        """Application of parentheses to XOR and NOT operators."""
        self.tmp.write("g1 := ~(e1 ^ e2)\n")
        self.tmp.flush()
        self.negation("xor")

    def test_not_and(self):
        """Application of parentheses to AND and NOT operators."""
        self.tmp.write("g1 := ~(e1 & e2)\n")
        self.tmp.flush()
        self.negation("and")

    def test_not_atleast(self):
        """Formula with negation of K/N operator."""
        self.tmp.write("g1 := ~@(2, [e1, e2, e3])\n")
        self.tmp.flush()
        self.negation("atleast", 3)

    def test_not_not(self):
        """Formula with negation of NOT operator."""
        self.tmp.write("g1 := ~(~e1)\n")
        self.tmp.flush()
        self.negation("not", 1)

    def combination(self, op_two, num_args=4):
        """Common cases for combination operator containing other formulas.

        The first operator must be K/N.
        The first two node arguments are associated with the K/N operator,
        the rest are assigned to the second operator.

        Args:
            op_two: The second operator of lower precedence.
            num_args: The number of arguments in the nested formula.
        """
        args = []
        for i in range(1, num_args + 1):
            args.append("e%d" % i)
        fault_tree = parse_input_file(self.tmp.name)
        assert_is_not_none(fault_tree)
        assert_equal(len(fault_tree.gates), 1)
        gate = fault_tree.gates["g1"].formula
        assert_equal(gate.num_arguments(), 3)
        assert_equal(gate.operator, "atleast")
        assert_equal(gate.node_arguments, args[:2])
        assert_equal(len(gate.f_arguments), 1)
        child_f = gate.f_arguments[0]
        assert_equal(child_f.num_arguments(), num_args - 2)
        assert_equal(child_f.operator, op_two)
        assert_equal(child_f.node_arguments, args[2:])

    def test_atleast_or(self):
        """Formula with K/N containing OR operator."""
        self.tmp.write("g1 := @(2, [e1, e2, e3 | e4])\n")
        self.tmp.flush()
        self.combination("or")

    def test_atleast_xor(self):
        """Formula with K/N containing XOR operator."""
        self.tmp.write("g1 := @(2, [e1, e2, e3 ^ e4])\n")
        self.tmp.flush()
        self.combination("xor")

    def test_atleast_and(self):
        """Formula with K/N containing AND operator."""
        self.tmp.write("g1 := @(2, [e1, e2, e3 & e4])\n")
        self.tmp.flush()
        self.combination("and")

    def test_atleast_not(self):
        """Formula with K/N containing NOT operator."""
        self.tmp.write("g1 := @(2, [e1, e2, ~e3])\n")
        self.tmp.flush()
        self.combination("not", 3)

    def test_atleast_atleast(self):
        """Formula with K/N containing K/N operator."""
        self.tmp.write("g1 := @(2, [e1, e2, @(2, [e3, e4, e5])])\n")
        self.tmp.flush()
        self.combination("atleast", 5)
