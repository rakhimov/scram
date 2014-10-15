#########################
SCRAM Fault Tree Analysis
#########################

This is a short description of the fault tree analysis implemented in
SCRAM.

Currently Supported Gate Types
==============================

- AND
- OR
- NOT
- NOR
- NAND
- XOR
- NULL
- INHIBIT
- ATLEAST

Currently Supported Symbols
==============================

- TransferIn
- TransferOut


Currently Supported Event Types
===============================

- Top
- Intermediate
- Basic
- House
- Undeveloped
- Conditional

.. note::
    Top and intermediate events are gates of the acyclic fault tree.

The Algorithm to Generate Minimal Cut Sets
===========================================

- Walk through the tree level by level starting from the root to leaves.
- Update cut sets after each level.
- Remove non-minimal cut sets according to rules.
- Remove cut sets that are larger than the specified maximum order.
- Remove cut sets with lower than cutoff probability. [not implemented]

More information in :ref:`mcs_algorithm`.


Probability Calculations
============================================

Many assumptions may be applied for calculation of set and total
probabilities. Some of them are:

- Independence of events (dependence may be simulated by common cause).
- Rare event approximation (must be enforced by a user).
- Min-Cut-Upper Bound Approximation (must be enforced by a user).
- Cut-off probability for minimal cut sets (the default value is 1e-8).
- Brute force probability calculation if the rare event approximation is not
  good enough. This brute force calculation may be expensive and require
  much more time. (the default method for probability calculations).

More information in :ref:`prob_calc`.

.. note::
    For most calculations, rare event approximation and event
    independence may be applied satisfactorily. However, if the rare event
    approximation produces too large probability, SCRAM can use the upper bound
    method. An appropriate warning will be given even if the user enforces
    the rare event approximation. It is suggested that the maximum number of
    series should be around 4-8. The default value includes all the terms.
    There are N sums and :math:`2^N` terms for N minimal cut sets.
