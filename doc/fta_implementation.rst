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

Representation of INHIBIT, Undeveloped, and Conditional
=======================================================

This gate and event types are not directly supported by the input format
based on OpenPSA MEF. Indeed, these types are treated just like AND gate and
Basic event respectively; therefore, the description of these types is
supported through OpenPSA MEF "attribute" element for gates and events.
The attribute name "flavor" is used to indicated the different representation
of the event as shown in the description bellow.

INHIBIT
-------
Add this XML line to AND gate description: :literal:`<attributes> <attribute name="flavor" value="inhibit"/> </attributes>`

Undeveloped
-----------
Add this XML line to basic event description: :literal:`<attributes> <attribute name="flavor" value="undeveloped"/> </attributes>`

Conditional
-----------
Add this XML line to basic event description: :literal:`<attributes> <attribute name="flavor" value="conditional"/> </attributes>`


The Algorithm to Generate Minimal Cut Sets
===========================================

- Walk through the tree level by level starting from the root to leaves.
- Update cut sets after each level.
- Remove non-minimal cut sets according to rules.
- Remove cut sets that are larger than the specified maximum order.
- Remove cut sets with lower than cutoff probability. [not implemented]

UNITY and NULL Cases
====================

The analyzed minimal cut sets may result in NULL or UNITY state, which may
indicate guaranteed success or failure. This cases are handled as special and
given an appropriate messages and probabilities. For example, UNITY state will
show only one minimal cut set of order 0 but probability 1.

More information in :ref:`mcs_algorithm`.
