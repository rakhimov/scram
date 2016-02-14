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

"""Fault tree classes and common facilities."""

class Node(object):
    """Representation of a base class for a node in a fault tree.

    Attributes:
        name: A specific name that identifies this node.
        parents: A set of parents of this node.
    """

    def __init__(self, name):
        """Constructs a new node with a unique name.

        Note that the tracking of parents introduces a cyclic reference.

        Args:
            name: Identifier for the node.
        """
        self.name = name
        self.parents = set()

    def is_common(self):
        """Indicates if this node appears in several places."""
        return len(self.parents) > 1

    def is_orphan(self):
        """Determines if the node is parentless."""
        return not self.parents

    def num_parents(self):
        """Returns the number of unique parents."""
        return len(self.parents)

    def add_parent(self, gate):
        """Adds a gate as a parent of the node.

        Args:
            gate: The gate where this node appears.
        """
        assert gate not in self.parents
        self.parents.add(gate)


class BasicEvent(Node):
    """Representation of a basic event in a fault tree.

    Attributes:
        prob: Probability of failure of this basic event.
    """

    def __init__(self, name, prob):
        """Initializes a basic event node.

        Args:
            name: Identifier of the node.
            prob: Probability of the basic event.
        """
        super(BasicEvent, self).__init__(name)
        self.prob = prob

    def to_xml(self):
        """Produces OpenPSA MEF XML definition of the basic event."""
        return ("<define-basic-event name=\"" + self.name + "\">\n"
                "<float value=\"" + str(self.prob) + "\"/>\n"
                "</define-basic-event>\n")

    def to_shorthand(self):
        """Produces the shorthand definition of the basic event."""
        return "p(" + self.name + ") = " + str(self.prob) + "\n"


class HouseEvent(Node):
    """Representation of a house event in a fault tree.

    Attributes:
        state: State of the house event ("true" or "false").
    """

    def __init__(self, name, state):
        """Initializes a house event node.

        Args:
            name: Identifier of the node.
            state: Boolean state string of the constant.
        """
        super(HouseEvent, self).__init__(name)
        self.state = state

    def to_xml(self):
        """Produces OpenPSA MEF XML definition of the house event."""
        return ("<define-house-event name=\"" + self.name + "\">\n"
                "<constant value=\"" + self.state + "\"/>\n"
                "</define-house-event>\n")

    def to_shorthand(self):
        """Produces the shorthand definition of the house event."""
        return "s(" + self.name + ") = " + str(self.state) + "\n"


class CcfGroup(object):
    """Representation of CCF groups in a fault tree.

    Attributes:
        name: The name of an instance CCF group.
        members: A collection of members in a CCF group.
        prob: Probability for a CCF group.
        model: The CCF model chosen for a group.
        factors: The factors of the CCF model.
    """

    def __init__(self, name):
        """Constructs a unique CCF group with a unique name.

        Args:
            name: Identifier for the group.
        """
        self.name = name
        self.members = []
        self.prob = None
        self.model = None
        self.factors = None

    def to_xml(self):
        """Produces OpenPSA MEF XML definition of the CCF group."""
        mef_xml = ("<define-CCF-group name=\"" + self.name + "\""
                   " model=\"" + self.model + "\">\n<members>\n")
        for member in self.members:
            mef_xml += "<basic-event name=\"" + member.name + "\"/>\n"
        mef_xml += ("</members>\n<distribution>\n<float value=\"" +
                    str(self.prob) + "\"/>\n</distribution>\n")
        mef_xml += "<factors>\n"
        assert self.model == "MGL"
        level = 2
        for factor in self.factors:
            mef_xml += ("<factor level=\"" + str(level) + "\">\n"
                        "<float value=\"" + str(factor) + "\"/>\n</factor>\n")
            level += 1

        mef_xml += "</factors>\n</define-CCF-group>\n"
        return mef_xml
