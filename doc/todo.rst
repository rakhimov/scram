###############################
TODO List for the SCRAM Project
###############################

Low Hanging Fruits
==================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_
- `TODO list in the code <http://scram-pra.org/api/todo.xhtml>`_
- `Bugs and Issues <https://github.com/rakhimov/scram/blob/develop/doc/bugs.rst>`_


.. note:: The following To-Do items are features with high uncertainties.
          Upon becoming more certain (realistic, clear, ready, doable),
          the items should graduate to GitHub issues.

.. note:: Relative, subjective importance within groups is tagged in *italics*.


Enhancements and Capabilities
=============================

Major
-----

- Incorporation of an event tree analysis. *Moderate*
- Event tree chaining. *Moderate*
- Event tree fault tree linking. *High*
- Incorporation of alignments. *Moderate*

    * Maintenance
    * Guaranteed Success/Failure

- Consequences and Consequence groups. *Moderate*
- Substitutions to represent
  Delete Terms, Recovery Rules, and Exchange Event. *Moderate*
- Sensitivity analysis. *Moderate*
- Dynamic Fault Tree Analysis. *Moderate*


Minor
-----

- Incorporation of cut-offs for ZBDD. *High*
- Advanced variable ordering and reordering heuristics for BDD. *Low*
- Joint importance reliability factor. *Low*
- Analysis for all system gates (qualitative and quantitative).
  Multi-rooted graph analysis. *Low*
- Importance factor calculation for gates (formulas). *Low*
- Uncertainty analysis for importance factors. *Moderate*
- The Open-PSA MEF Support:

    * Expressions. *Moderate*
    * "Include directive" in input files to include other input files. *Low*
    * Cardinality/Imply/IFF gates. *Low*


Platform Support
================

- Installation Package for Windows.
- Installation Package for Mac OS X.


Code
====

- More tests for expressions. *Moderate*

- More tests for preprocessing techniques. *High*

    * Graph equality tests.

- Benchmark tests:

    * HIPPS (periodic tests)
