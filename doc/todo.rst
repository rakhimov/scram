#################################
TODO list for SCRAM Project
#################################

.. note::
    Relative importances within groups are given in *italics*.

Currently Low Hanging Fruits
============================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_

- `Todo list in the code <https://rakhimov.github.io/scram/api/todo.html>`_

- XML based analysis configuration input file. *High*

  * Schema
  * Tests

- XML based output file. *High*

  * Schema
  * Tests

- Minimal cut set input file as an alternative to a tree file. *Moderate*

- Option to set the seed for Monte Carlo simulations. *Low*

- Option to run analysis without full basic events description. *Moderate*


Major Enhancements and Capabilities
===================================

- Boolean formula rewriting for fault trees. *High*

- Incorporation of alignments. *Moderate*

    * Maintenance
    * Guaranteed Success/Failure

- OpenPSA-like XML input format. *High*

  * Reduce tests for input format and parser.
  * Create Include feature.

    + Test for non-existent file, circular inclusion(direct, indirect).

  * Subtree Analysis of a Fault Tree.

- Binary Decision Diagram (BDD) based Algorithms. *High*

- Zero-Suppressed BDD (ZBDD) based Algorithms. *High*

- Sensitivity analysis. *Moderate*

- Incorporation of an event tree analysis. *Moderate*

- Dynamic Fault Tree Analysis. *Moderate*


Minor Enhancements and Capabilities
===================================

- OpenPSA MEF Support:

  * Nested formula for gates. *Moderate*
  * Expressions for basic events. *Moderate*
  * Include directive in input files to include other input files. *Low*

- Improve fault tree generator. *Low*

  * Advanced gates
  * CCF groups

- Create cut set generator. *Low*


GUI Development: Moderate Importance
====================================

- Layout

- Fault Tree Construction and Representation

- Description of events

- Event Tree Construction and Representation

- Analysis configuration

- Running analyses

- View of analysis results

- Common cause group construction


Documentation: Moderate Importance
==================================

- Getting Started


Platform Support
================

- Compiling with Mingw-w64 for Windows. *High*

- Installation Package for Windows. *High*

- Installation Package for Mac OS X. *Moderate*

- Installation Package for Ubuntu (PPA/DEB). *Low*


Coding Specific
===============

- More tests for expressions. *Moderate*

- Benchmark tests:

  * HIPPS
  * BSCU
