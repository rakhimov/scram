#################################
TODO list for SCRAM Project
#################################

Relative importances within groups are given in *italics*.

Currently Low Hanging Fruits
============================

- Correct Transfer behavior. *Moderate*

- Define output format. *Moderate*

    * Table like spacing for output numbers
    * Take a look at output formats of RiskMan and Isograph
    * Option for CSV output for spreadsheet applications
    * Save minimal cut sets for later analysis

- Minimal cut set input file as an alternative to a tree file. *Moderate*

- New gates, event types, and probability types. *High*

    * Probabilities as a function of time (lambda). *High*

- Monte Carlo Simulations. *Moderate*

    * Uncertainty analysis
    * Sensitivity analysis
    * Sampling techniques
    * Distributions
    * Latin Hypercube sampling technique
    * Refer to OpenFTA: Minimal cut set generation through MC

- Reformat documentation into rst. *Low*

- Analysis of a transfer sub-tree without a main file. *Low*

- Improve fault tree generator. *Low*


Major Enhancements and Capabilities
===================================

- Incorporation of an event tree analysis. *Moderate*

- Incorporation of CCF. *High*

    * Multiple Greek Letter system
    * Alpha system
    * Beta system

- Incorporation of alignments. *Moderate*

    * Maintenance
    * Guaranteed Success/Failure


Minor Enhancements and Capabilities
===================================

- Other input styles : XML, json, sql. *Low*
- GUI considerations for building trees. *Moderate*
- Explore conditional probabilities through CCF. *Low*


Documentation: Moderate Importance
==================================

- Getting Started
- Building/Installing
- Installation instructions for various platforms. *Low*
- Simple Walkthrough Example
- Reference Papers
- Background Theory

Coding Specific
===================

- Time and space complexity of all algorithms used in this package. *Moderate*
