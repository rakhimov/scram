###########
Description
###########

Command-line Usage
==================

The user supplies a configuration file, input files,
and analysis configurations via command-line.
Various types of analyses can be performed on the model,
for example, fault tree, event tree, uncertainty, etc.
The user must specify the kinds of analysis
and appropriate parameters for the analyses.
For example, if FTA is requested (the default option),
the maximum order of products can be specified for faster analysis.
The default values of the parameters can be used for initial runs
before adjusting them for accuracy and performance.

Since a single model may contain several analysis targets (fault trees, event trees, etc.),
a judicious split of analysis constructs into files
is essential to control the analysis target.
In other words, SCRAM analyzes all possible targets in user-supplied input files;
there is no fine-grained analysis configuration per target.

- Run ``scram --help`` to see all the flags and options for analysis.

- All XML files are validated against the RELAX NG schema
  before any initialization and validation of analysis specific constructs.
  This automatic validation may give unpleasant, cryptic, low-level error messages
  for invalid XML files.

- The analysis run may take considerable time and computing resources
  depending on the complexity of the model.
  You can adjust SCRAM flags and parameters to reduce these demands.

- All analyses can be run with logging (``--verbosity``).
  The logging system outputs useful information
  for figuring out limiting bottlenecks.
  This information can be used for debugging and testing purposes.


Analysis
========

- :ref:`fault_tree_analysis`
- :ref:`common_cause_failure`
- :ref:`probability_analysis`
- :ref:`uncertainty_analysis`
- :ref:`event_tree_analysis`
