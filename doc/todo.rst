###############################
TODO List for the SCRAM Project
###############################

Low Hanging Fruits
==================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_
- `TODO list in the code <https://rakhimov.github.io/scram/api/todo.html>`_


.. note:: This To-Do list is for features with high uncertainties.
          Upon becoming more certain (realistic, clear, ready, doable),
          the items should graduate to GitHub issues.

.. note:: Relative, subjective importance within groups is given in *italics*.


Major Enhancements and Capabilities
===================================

- Incorporation of cut-offs for MOCUS. *High*
- Incorporation of cut-offs for BDD. *Moderate*
- Incorporation of cut-offs for ZBDD. *Moderate*
- Advanced variable ordering heuristics for BDD. *High*
- Sensitivity analysis. *Moderate*
- Incorporation of an event tree analysis. *Moderate*
- Event tree chaining. *Moderate*
- Event tree fault tree linking. *High*
- Incorporation of alignments. *Moderate*

    * Maintenance
    * Guaranteed Success/Failure

- Consequences and Consequence groups. *Moderate*
- Substitutions to represent
  Delete Terms, Recovery Rules, and Exchange Event. *Moderate*
- Dynamic Fault Tree Analysis. *Moderate*


Minor Enhancements and Capabilities
===================================

- Joint importance reliability factor. *Low*
- Analysis for all system gates (qualitative and quantitative).
  Multi-rooted graph analysis. *Low*
- Importance factor calculation for gates (formulas). *Low*
- Uncertainty analysis for importance factors. *Moderate*
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

- Installation Package for Windows.
- Installation Package for Mac OS X.


Coding Specific
===============

- More tests for expressions. *Moderate*

- More tests for preprocessing techniques. *High*

    * Graph equality tests.

- Benchmark tests:

    * HIPPS (periodic tests)
    * BSCU (numerical expressions)
