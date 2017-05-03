.. _fault_tree_analysis:

###################
Fault Tree Analysis
###################

Fault tree analysis [FTA]_ is one of the core analyses.
Other analyses may require or depend on the fault tree results or constructs.
Fault trees employ various types of gates (Boolean connectives) and events
to represent Boolean formulas and to model systems for analysis.
SCRAM automatically discovers top-events (graph roots)
and runs all requested analysis kinds with those events.
The following is a description of some analysis goals
and their respective control parameters (knobs).

#. Find minimal cut sets or prime implicants. *Probability input is optional*

   - Cut-off probability for products. *Unused*
   - Maximum order for products for faster calculations.

#. Find the total probability of the top event
   and importance values for basic events. *Only if probability input is provided*

   - Cut-off probability for products. *Unused*
   - The rare event or MCUB approximation. *Optional*
   - Mission time that is used to calculate probabilities.


Analysis Algorithms
===================

- :ref:`preprocessing`
- :ref:`fta_algorithms`
- :ref:`probability_analysis`


Supported Gate Types
====================

- AND
- OR
- NOT
- NOR
- NAND
- XOR
- NULL
- INHIBIT
- VOTE


Supported Event Types
=====================

- Top
- Intermediate
- Basic
- House
- Undeveloped
- Conditional

.. note:: Top and intermediate events are gates of an acyclic "fault-tree" graph ([PDAG]_).

.. note::
    Transfer-in and Transfer-out symbols,
    mostly employed by graphical front-ends,
    are unnecessary with an input format based on the Open-PSA MEF
    since the "fault-tree" is properly treated as a graph/container.


Representation of INHIBIT, Undeveloped, and Conditional
=======================================================

These gate and event types are not directly supported
by the input format based on the Open-PSA [MEF]_.
Indeed, these types are treated just like AND gate and Basic event respectively;
therefore, the description of these types
can be given through the Open-PSA MEF "attribute" element for gates and events.
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
