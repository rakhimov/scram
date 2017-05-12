###############################
TODO List for the SCRAM Project
###############################

Low Hanging Fruits
==================

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_
- `TODO list in the code <https://scram-pra.org/api/todo.xhtml>`_
- `Bugs and Issues <https://github.com/rakhimov/scram/blob/develop/doc/bugs.rst>`_


.. note:: The following To-Do items are features with high uncertainties.
          Upon becoming more certain (realistic, clear, ready, doable),
          the items should graduate to GitHub issues.

.. note:: Relative, subjective importance within groups is tagged in *italics*.
          The order of items is arbitrary (i.e., irrelevant).


Enhancements and Capabilities
=============================

Major
-----

- Sensitivity analysis. *Moderate*
- Dynamic fault tree analysis. *High*
- Systems with loops (risk network, strongly-connected components). *Low*
- Markov chains (numerical and simulation-based analyses). *Moderate*
- Reliability block diagrams. *Moderate*


Minor
-----

- Quantitative analysis with BDD w/o qualitative analysis. *Moderate*
- Event-tree analysis shadow-variables optimizations. *High*
- Incorporation of cut-offs (probability, contribution, dynamic) for ZBDD. *Moderate*
- Advanced variable ordering and reordering heuristics for BDD. *Moderate*
- Joint importance reliability factor. *Low*
- Analysis for all system gates (qualitative and quantitative).
  Multi-rooted graph analysis. *Low*
- Importance factor calculation for gates (formulas). *Low*
- Uncertainty analysis for importance factors. *Moderate*
- Cardinality/Imply/IFF gates. *Low*
- Generalization of parameter units and dimensional analysis. *Low*


Code
====

- Python front-end. *Moderate*

- More tests for expressions. *Moderate*

- More tests for preprocessing techniques. *High*

    * Graph equality tests.
