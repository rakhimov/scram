#!/usr/bin/env bash

# Runs Lizard to get cyclomatic complexity report for SCRAM source files.
# Exludes env.cc, version.cc, and other irrelevant for CCN files.
# This script must be run in scram/src directory

lizard -s cyclomatic_complexity -L 60 -a 5 \
  scram.cc \
  cycle.cc \
  error.cc \
  logger.cc \
  relax_ng_validator.cc \
  xml_parser.cc \
  grapher.cc \
  reporter.cc \
  element.cc \
  expression.cc \
  event.cc \
  ccf_group.cc \
  fault_tree.cc \
  model.cc \
  config.cc \
  settings.cc \
  initializer.cc \
  risk_analysis.cc \
  boolean_graph.cc \
  preprocessor.cc \
  mocus.cc \
  bdd.cc \
  zbdd.cc \
  fault_tree_analysis.cc \
  probability_analysis.cc \
  importance_analysis.cc \
  uncertainty_analysis.cc \
