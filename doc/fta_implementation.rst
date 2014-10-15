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
