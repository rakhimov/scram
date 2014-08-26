#################################
TODO list for SCRAM Project
#################################

.. note::
    Relative importances within groups are given in *italics*.

Currently Low Hanging Fruits
============================

- `Issues on GitHub <https://github.com/rakhimov/SCRAM/issues>`_

- Minimal cut set input file as an alternative to a tree file. *Moderate*

    * Save minimal cut sets for later analysis

- Monte Carlo Simulations. *Moderate*

    * Uncertainty analysis
    * Sensitivity analysis
    * Sampling techniques
    * Distributions
    * Latin Hypercube sampling technique

- Improve fault tree generator. *Low*

- Create cut set generator. *Low*


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

  * Remove custom input format and parser.
  * Reduce tests for input format and parser.
  * Add labels to all members.
  * Add Attributes struct to all members.
  * Deprecate Transfer tests. Create Include tests instead.

    + Test for non-existent file, circular inclusion(direct, indirect).

  * SubTree Analysis of a Fault Tree.

- GUI development. *High*


Minor Enhancements and Capabilities
===================================


Documentation: Moderate Importance
==================================

- Getting Started
- Building/Installing
- Installation instructions for various platforms
- Simple Walk-through Example:

    * Various ways of writing input files.
    * Separation of inputs.

Coding Specific
===================
