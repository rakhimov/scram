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
"""Tests to command-line SCRAM with correct and incorrect arguments."""

import os
from subprocess import call

import pytest


def test_empty_call():
    """Tests a command-line call without any arguments."""
    cmd = ["scram"]
    assert call(cmd) == 1


def test_info_help_calls():
    """Tests general information calls about SCRAM."""
    cmd = ["scram", "--help"]
    assert call(cmd) == 0


def test_info_version_calls():
    """Test version information."""
    cmd = ["scram", "--version"]
    assert call(cmd) == 0


def test_fta_no_prob(tmpdir):
    """Tests calls for fault tree analysis without probability information."""
    fta_no_prob = "./input/fta/correct_tree_input.xml"
    # Test calculation calls
    cmd = ["scram", fta_no_prob]
    assert call(cmd) == 0
    out_temp = str(tmpdir / "output_temp.xml")
    cmd.append("-o")
    cmd.append(out_temp)
    assert call(cmd) == 0  # report into an output file
    if os.path.isfile(out_temp):
        os.remove(out_temp)


@pytest.mark.parametrize(
    'cmd, status',
    [
        (["--validate"], True),
        # Test the incorrect limit order
        (["-l", "-1"], False),
        # Invalid argument type for an option
        (["-l", "string_for_int"], 1),
        # Test the limit order no minimal cut sets.
        # This was an issue #17. This should not throw an error anymore.
        (["-l", "1"], True),
        # Test the incorrect cut-off probability
        (["--cut-off", "-1"], False),
        (["--cut-off", "10"], False),
        # Test conflicting algorithms
        (["--zbdd", "--bdd"], False),
        # Test the application of the rare event and MCUB at the same time
        (["--rare-event", "--mcub"], False),
        # Test the rare event approximation
        (["--rare-event"], True),
        # Test the MCUB approximation
        (["--mcub"], True),
        # Test the uncertainty
        (["--uncertainty", "true", "--num-bins", "20", "--num-quantiles", "20"],
         True),
        # Test calls for prime implicants
        (["--prime-implicants", "--mocus"], False),
        (["--prime-implicants", "--rare-event"], False),
        (["--prime-implicants", "--mcub"], False)
    ])
def test_fta_calls(cmd, status):
    """Tests calls for full fault tree analysis."""
    fta_input = "./input/fta/correct_tree_input_with_probs.xml"
    ret = call(["scram", fta_input] + cmd)
    if not isinstance(status, bool):
        assert ret == status
    elif status:
        assert ret == 0
    else:
        assert ret != 0


def test_config_file_output(tmpdir):
    """Tests calls with configuration files."""
    # Test with a configuration file
    config_file = "./input/fta/full_configuration.xml"
    out_temp = str(tmpdir / "output_temp.xml")
    cmd = ["scram", "--config-file", config_file, "-o", out_temp]
    assert call(cmd) == 0
    if os.path.isfile(out_temp):
        os.remove(out_temp)


def test_config_file_clash():
    """Test the clash of files from configuration and command-line."""
    config_file = "./input/fta/full_configuration.xml"
    cmd = [
        "scram", "--config-file", config_file,
        "input/fta/correct_tree_input_with_probs.xml"
    ]
    assert call(cmd) != 0


@pytest.mark.parametrize('level,status',
                         [(-1, False), (0, True), (1, True), (2, True),
                          (3, True), (4, True), (5, True), (6, True), (7, True),
                          (8, False), (10, False), (100, False), (1e6, False)])
def test_logging(level, status):
    """Tests invokation with logging."""
    fta_input = "./input/fta/correct_tree_input_with_probs.xml"
    ret = call(["scram", fta_input, "--verbosity", str(level)])
    if status:
        assert ret == 0
    else:
        assert ret != 0
