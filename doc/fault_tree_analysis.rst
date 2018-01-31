.. _fault_tree_analysis:

###################
Fault Tree Analysis
###################

Fault tree analysis [FTA]_ is one of the core analyses.
Other analyses may require or depend on the fault-tree analysis results or constructs.
Fault trees employ various types of gates (Boolean connectives) and events
to represent Boolean formulas and to model systems for analysis.
SCRAM automatically discovers top-events (graph roots)
and runs all requested analysis kinds with those events.
The following is a description of some analysis goals
and their respective control parameters (knobs).

#. Find minimal cut sets or prime implicants. *Probability input is optional*

   - Cut-off probability for products. *Unused*
   - Maximum order for products for faster calculations.

#. Find the total probability of a top event
   and importance values for basic events. *Only if probability input is provided*

   - Cut-off probability for products. *Unused*
   - The rare event or MCUB approximation. *Optional*
   - Mission time that is used to calculate probabilities.


Analysis Algorithms
===================

- :ref:`preprocessing`
- :ref:`fta_algorithms`
- :ref:`probability_analysis`


Supported Gate and Event Types
==============================

All the event types and Boolean connectives described in the Open-PSA [MEF]_ are supported.
In addition,
SCRAM currently "abuses" the Open-PSA [MEF]_ attributes mechanism
to add inessential (for analysis) information or flavors to events.
This approach still ensures that the input file is portable across tools.


Undeveloped
-----------

Add this XML line to a basic event description:
:literal:`<attributes> <attribute name="flavor" value="undeveloped"/> </attributes>`
