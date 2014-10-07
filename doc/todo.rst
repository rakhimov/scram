#################################
TODO list for SCRAM Project
#################################

.. note::
    Relative importances within groups are given in *italics*.

Currently Low Hanging Fruits
============================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_

- Uncertainty analysis. *Moderate*

- XML based analysis configuration input file. *High*

  * Schema

- XML based output file. *High*

  * Schema

- Minimal cut set input file as an alternative to a tree file. *Moderate*


Major Enhancements and Capabilities
===================================

- Incorporation of an event tree analysis. *Moderate*

- Dynamic Fault Tree Analysis. *Moderate*

- Incorporation of CCF. *High*

    * Multiple Greek Letter system
    * Alpha system
    * Beta system
    * Phi system

- Incorporation of alignments. *Moderate*

    * Maintenance
    * Guaranteed Success/Failure

- OpenPSA-like XML input format. *High*

  * Reduce tests for input format and parser.
  * Create Include feature.

    + Test for non-existent file, circular inclusion(direct, indirect).

  * Subtree Analysis of a Fault Tree.

- GUI development. *High*

- Sensitivity analysis. *Moderate*


Minor Enhancements and Capabilities
===================================

- OpenPSA MEF Support:

  * Nested formula for gates. *Moderate*

  * Expressions for basic events. *Moderate*

- Importance Analysis: *High*

  * Fussel-Vesely

  * Birnbaum

  * Risk Reduction Worth

  * Risk Achievement Worth

- Improve fault tree generator. *Low*

- Create cut set generator. *Low*


Documentation: Moderate Importance
==================================

- Getting Started

- Simple Walk-through Examples/Tutorials:

    * Various ways of writing input files.
    * Separation of inputs.


Platform Support
================

- Compiling with Mingw-w64 for Windows. *High*

- Installation Package for Windows. *High*

- Installation Package for Mac OS X. *Moderate*

- Installation Package for Ubuntu (PPA/DEB). *Low*


Coding Specific
===============

- More exception types as needed:  *Moderate*

  * Invalid Argument

  * Illegal Operation

  * Logic Error

  * Double Definition

- Lower the Cyclomatic complexity.  *High*

- Logging system. *Moderate*

- Automated performance testing: *Moderate*

  * Build mode dependent (Debugging/Non-debugging).

  * Must be run optionally.
