###############################
TODO List for the SCRAM Project
###############################

.. note:: Relative, subjective importance within groups is given in *italics*.


Low Hanging Fruits
==================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_

- `TODO list in the code <https://rakhimov.github.io/scram/api/todo.html>`_


Major Enhancements and Capabilities
===================================

- Boolean formula rewriting for fault trees. *High*
- Binary Decision Diagram (BDD) based algorithms. *High*
- Zero-Suppressed BDD (ZBDD) based algorithms. *High*
- Incorporation of cut-offs for MOCUS. *High*
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

    * Expressions. *Moderate*
    * Include directive in input files to include other input files. *Low*
    * Cardinality/Imply/IFF gates. *Low*

- Shorthand format to XML converter. *Low*

    * Arithmetic equation interpreter for basic events.

- Create a cut-set generator. *Low*


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

    * HIPPS (periodic tests)
    * BSCU (numerical expressions)
