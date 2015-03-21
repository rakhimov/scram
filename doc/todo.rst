###########################
TODO list for SCRAM Project
###########################

.. note::
    Relative subjective importances within groups are given in *italics*.

Currently Low Hanging Fruits
============================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_

- `Todo list in the code <https://rakhimov.github.io/scram/api/todo.html>`_

- Option to run analysis without full basic event description. *Moderate*


Major Enhancements and Capabilities
===================================

- Boolean formula rewriting for fault trees. *High*

- OpenPSA-like XML input format. *High*

  * Reduce tests for input format and parser.
  * Create Include feature.

    + Test for non-existent file, circular inclusion(direct, indirect).

  * Subtree Analysis of a Fault Tree.

- Binary Decision Diagram (BDD) based Algorithms. *High*

- Zero-Suppressed BDD (ZBDD) based Algorithms. *High*

- Sensitivity analysis. *Moderate*

- Incorporation of an event tree analysis. *Moderate*

- Event tree chaining. *Moderate*

- Event tree fault tree linking. *High*

- Incorporation of alignments. *Moderate*

    * Maintenance
    * Guaranteed Success/Failure

- Consequences and Consequence groups. *Moderate*

- Substitutions to represent Delete Terms, Recovery Rules, and Exchange Event. *Moderate*

- Dynamic Fault Tree Analysis. *Moderate*


Minor Enhancements and Capabilities
===================================

- OpenPSA MEF Support:

  * Boolean formula. *Moderate*
  * Expressions. *Moderate*
  * Include directive in input files to include other input files. *Low*

- Improve fault tree generator. *Low*

  * Advanced gates
  * CCF groups

- Create cut set generator. *Low*

- Public and private scopes for names of events and other elements. *Low*

- Cardinality/Imply/IFF gates. *Low*


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
