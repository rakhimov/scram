#!/usr/bin/env bash

# Runs Lizard to get cyclomatic complexity report for SCRAM source files.
# Exludes env.cc, version.cc, and other irrelevant for CCN files.
# This script must be run in scram/src directory

lizard -s cyclomatic_complexity \
  boolean_graph.cc \
  ccf_group.cc \
  config.cc \
  cycle.cc \
  element.cc \
  error.cc \
  event.cc \
  expression.cc \
  fault_tree_analysis.cc \
  fault_tree.cc \
  grapher.cc \
  initializer.cc \
  logger.cc \
  mocus.cc \
  model.cc \
  preprocessor.cc \
  probability_analysis.cc \
  random.cc \
  relax_ng_validator.cc \
  reporter.cc \
  risk_analysis.cc \
  scram.cc \
  settings.cc \
  uncertainty_analysis.cc \
  xml_parser.cc
