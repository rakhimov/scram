###################
Fault Tree Analysis
###################

Fault trees have various types of gates and events
to represent Boolean equation and systems for analysis [FTA]_.
However, extra information for visual or informative purposes for the analyst
may be irrelevant for the core analysis.
Support for more advanced types and event descriptions will be introduced as needed in SCRAM.


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

.. note:: Top and intermediate events are gates of an acyclic "fault-tree" graph ([PDAG]_).


Representation of INHIBIT, Undeveloped, and Conditional
=======================================================

These gate and event types are not directly supported
by the input format based on the OpenPSA [MEF]_.
Indeed, these types are treated just like AND gate and Basic event respectively;
therefore, the description of these types
can be given through the OpenPSA MEF "attribute" element for gates and events.
The attribute name "flavor" is used to indicate
the different representation of an event as shown in the description bellow.


INHIBIT
-------

Add this XML line to AND gate description:
:literal:`<attributes> <attribute name="flavor" value="inhibit"/> </attributes>`


Undeveloped
-----------

Add this XML line to basic event description:
:literal:`<attributes> <attribute name="flavor" value="undeveloped"/> </attributes>`


Conditional
-----------

Add this XML line to basic event description:
:literal:`<attributes> <attribute name="flavor" value="conditional"/> </attributes>`


Analysis Algorithms
===================

- :ref:`preprocessing`
- :ref:`mcs_algorithm`
- :ref:`prob_calc`
