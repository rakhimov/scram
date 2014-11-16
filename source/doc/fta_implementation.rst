#################################
SCRAM Fault Tree Analysis Support
#################################

Fault trees have various types of gates and events to represent Boolean
formula and the system under analysis. However, some extra information
may be irrelevant for the core analysis tool and used only for visual or
informative purposes for an analyst. Support for more advanced types
and event descriptions will be introduces as needed in SCRAM.

There are many algorithms that can be used in the fault tree analysis, and each
has its own advantages and drawbacks. More advanced algorithms will be
implemented for complex analysis.

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
===========================

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


Algorithms to Generate Minimal Cut Sets
=======================================

- MOCUS-like algebraic algorithm:

    * Walk through the tree gate by gate starting from the root to leaves.
    * Gather cut sets after each gate.
    * Remove non-minimal cut sets.
    * Remove cut sets that are larger than the specified maximum order.
    * Remove cut sets with lower than cutoff probability. [not implemented]

Algorithms to Calculate Probabilities
=====================================

- Algebraic expansion of the probability equation.

    * Consider only cut sets above the cut-off probability.
    * Generate Sylvester-Poincaré expansion.
    * Plug-in basic event probabilities.

UNITY and NULL Cases
====================

The analyzed minimal cut sets may result in NULL or UNITY state, which may
indicate guaranteed success or failure. This cases are handled as special and
given an appropriate messages and probabilities. For example, UNITY state will
show only one minimal cut set of order 0 but probability 1.

More information in :ref:`mcs_algorithm`.
