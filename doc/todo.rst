###############################
TODO List for the SCRAM Project
###############################

Low Hanging Fruits
==================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_
- `TODO list in the code <http://scram-pra.org/api/todo.html>`_


.. note:: The following To-Do items are features with high uncertainties.
          Upon becoming more certain (realistic, clear, ready, doable),
          the items should graduate to GitHub issues.

.. note:: Relative, subjective importance within groups is tagged in *italics*.


Major Enhancements and Capabilities
===================================

- Incorporation of cut-offs for MOCUS. *High*
- Incorporation of cut-offs for ZBDD. *Moderate*
- Advanced variable ordering heuristics for BDD. *Moderate*

    * Post-construction reordering.

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
- Arithmetic equation interpreter for basic events in shorthand format. *Low*
- OpenPSA MEF Support:

    * Expressions. *Moderate*
    * "Include directive" in input files to include other input files. *Low*
    * Cardinality/Imply/IFF gates. *Low*


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
    * BSCU (numerical expressions)


Technical Issues
----------------

- Copying Settings around is expensive (~100B)
- Abuse of smart pointers (shared pointers)
- Operator enum has "Gate" in its names
- IGate and Formula have 'type' field instead of 'operator' (reserved in C++)
- 'Atleast' vs. 'Vote' vs. 'K/N' vs. 'Combination'
- Bdd and Zbdd friendship is a design smell.
  (Access controlled Bdd::Consensus for Zbdd needs Bdd::Function outside of Bdd.)
- FetchTable() functions (Bdd/Zbdd) may not make sense semantically.
- gate->IsModule() vs. gate->module(). (Inconsistent with the rest of the code.)
- ConvertBddPI is an easy-to-get-wrong name. (There is ConvertBdd.)
- IGate in Boolean graph is confusing (There is Gate in event.h)
- Questionable explicit qualification rules for member functions.
- Parsing code for nested formulas in ``shorthand_to_xml.py`` is add-hoc (ugly, incorrect).
- Fault tree generator script is too complex.
- Fuzz tester runs are coupled.
  Can't run two jobs at the same time.
- Fuzz tester error collection and reporting are non-existent.
- Preprocessing contracts need review, update, and clarification.
- BooleanGraph must guarantee stable results (construction from PDAG).
- Migrate "Quick Installation" to "Installation" web page.
  (Windows installation is needed to get any value out of this migration.)
- The code is not well designed as public API.
  (Example design flaw: call analysis twice and get undefined behavior.)
- Virtually everything is under one namespace.
- Python code coverage is not continuously tracked through CI.
  (Problems with merging C++ and Python code coverage on Coveralls.)
