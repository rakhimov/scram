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
        """Produces OpenPSA MEF XML representation of the CCF group."""
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
