#!/usr/bin/env bash

# Runs Lizard to get cyclomatic complexity report for SCRAM source files.
# Exludes env.cc and version.cc files.
# This script must be run in scram/src directory

lizard -s cyclomatic_complexity \
  element.cc \
  error.cc \
  event.cc \
  fault_tree_analysis.cc \
  fault_tree.cc \
  gate.cc \
  grapher.cc \
  primary_event.cc \
  probability_analysis.cc \
  random.cc \
  relax_ng_validator.cc \
  reporter.cc \
  risk_analysis.cc \
  scram.cc \
  settings.cc \
  superset.cc \
  uncertainty_analysis.cc \
  xml_parser.cc
