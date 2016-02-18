.. _theory:

######
Theory
######

This section contains short descriptions of the analyses used in probabilistic risk assessment.
More information can be found in
:ref:`references`, [WASH1400]_, [FTACOURSE]_, [FTHB]_, [PRA]_.


**************************
Static Fault Tree Analysis
**************************

Fault Tree Analysis is a top-down deductive method
to understand the logic leading to the top undesired event or fault [FTA]_.
Boolean logic is used to combine events
that could lead to the undesired top event.
This analysis assumes that events are independent
and may incorporate Common Cause Failure for the analysis with dependent systems.
The analysis generates minimal cut sets or prime implicants,
importance factors of events,
probabilities of top event and gates.


Static Fault Tree Symbols, Gate and Event Types
===============================================

EVENTS
------

#. Top Event :
    Contains a description of the top-system-level fault or undesired event.
    The root of a fault tree.
    Top event only receives input from a logic gate.

#. Intermediate/Fault Event :
    Contains a description of a lower-level fault in the system.
    Intermediate events receive input from and provide outputs to a logic gate.

#. Basic Event :
    Contains a failure at the lowest level of examination,
    which has a potential to cause a fault in the system.
    This is a leaf of a fault tree,
    and it provides input to a logic gate.
    This event must have its probability defined.

#. Undeveloped Event :
    Similar to a basic event,
    but an undeveloped event has its own causes
    that may be analyzed by a separate fault tree.
    This event is not further developed
    either because of insufficient consequence
    or because information is unavailable.
    This event must have its probability defined
    and provide input to a logic gate.

#. House/Input/External Event :
    Contains a normal system operating input with the capability of causing a fault.
    This event is expected to occur with probability 0 or 1 only,
    or it is sometimes used as a switch of True or False.
    Provides input to a logic gate.
    May be used to switch off a sub-tree.

#. Conditional Event :
    Defines a specific condition or restriction
    that apply to any logic gate.
    Used mostly with Priority AND and Inhibit gates.
    The alternative name is a Conditional Qualifier.
    Conditions may be abnormal cases or environment.
    The probability of this event might be a conditional probability.


SYMBOLS
-------

#. Transfer In :
    This symbol indicates that the tree is expanded further
    at the occurrence of the corresponding Transfer Out symbol.
    This expansion may be described in another file or page.

#. Transfer Out :
    This symbol indicates that this portion of the tree must be attached
    at the corresponding Transfer In symbol of the main fault tree.


GATES
-----

#. AND :
    Output fault occurs if all the input faults occur.

#. OR :
    Output fault occurs if at least one of the input faults occur.

#. Exclusive OR (XOR) :
    Output fault occurs if exactly one of the two input faults occurs.
    (A XOR B) = (A AND B') OR (A' AND B).
    In other words, the result is one or the other but not both.
    This gate can only have two inputs.
    For several inputs,
    the output fault occurs if odd number of faults occur.
    This can be simulated by chaining XOR gates if needed.

#. Inhibit :
    This gate is a special case of the AND gate.
    Output fault occurs
    if the single input fault occurs in the presence of an enabling condition
    (the enabling condition is represented by a Conditioning Event drawn to the right of the gate.).
    This gate restricts input events to only two events.

#. Combination/Voting/VOTE/Atleast/K-out-of-N(K/N) :
    Output fault occurs if **m** out of the **n** input events occurs.
    The **m** input events need not to occur simultaneously.
    The output occurs if at least **m** events occur.
    **m** is more than 1, and **n** is more than **m**.
    The **m** can also be called **vote number**.

#. NOT :
    Output fault occurs if the input event does not occur.
    This logic leads to non-coherent trees,
    for which non-occurrence or success of events
    may lead to occurrence of the main undesirable event.

#. NAND :
    NOT AND gate.
    Indicates that the output occurs
    when at least one of the input events is absent (does not occur or fail).
    This may lead to non-coherent trees.

#. NOR :
    NOT OR gate.
    Indicates that the output occurs
    when all the input events are absent.
    This may also lead to non-coherent trees.

#. NULL/Pass-Through :
    Non-essential.
    Only one input is allowed.
    Used in applications with GUI to allow more description or alignment.


OTHER
-----

#. Dormant Failure :
    Failures that are not detected by themselves
    and need secondary specific actions or failures to occur.
    This is a special case of a primary event
    that may fail with no visible external effects.
    May be treated as a basic event for primitive analysis.


*******************
Event Tree Analysis
*******************

Event Tree Analysis is a bottom-up approach
to quantify the risk resulting from an initiating event [ETA]_.
The tree is branched into conditionally independent,
mutually exclusive cases,
which lead to several final scenarios, outcomes, or end states.
This analysis is conceptually useful
when the system incorporates sequentially occurring events.

Most of the time,
there are two branches for success and failure cases,
but there may be more as long as the events are mutually exclusive.
Probabilities of intermediate cases can be calculated
with fault trees or assigned manually,
and they must sum to 1 for mutually exclusive and independent branches.


Fault Tree Linking
==================

If the original assumption of independent branches does not hold,
an event tree branches can be linked to corresponding gates in fault trees,
and the final tree is analyzed as a big fault tree.


********************
Common Cause Failure
********************

If events are not statistically independent,
common cause or mode analysis is performed
to account for the failure of multiple elements
at the same time or within a short period [CCF]_.
These common mode failures may be due to
the same manufacture flaws and design,
environment, working conditions,
maintenance, quality control,
normal wear and tear, and many other factors.
Several models are used to quantify the common cause failures.
The components in the same common cause group must be described by the same probability.
The exact formulas to compute factors are given in NRC [NUREG0492]_.


Beta System
===========

Beta systems assume that if common cause failure occurs,
all components in the group fail.
The components can fail independently,
but multiple independent failures are ignored.


Multiple Greek Letters(MGL) System
==================================

MGL is a generalization of Beta system.
MGL describes several conditional factors
that quantify the failure of the certain number of components due to common cause,
so the number of factors can be up to the number of components.
The factor for **k** number of elements indicates
failure of **k** or *more* components due to common cause.


Alpha System
============

This system is similar to MGL,
but the factor for **k** number of elements indicates
failure of *exactly* **k** number of elements due to common cause.


Phi System
==========

Phi system is the same as MGL and Alpha systems
except that the factors indicate
direct probability distribution of the common cause.
The phi factors must sum to 1.


********************
Uncertainty Analysis
********************

Uncertainty quantification is performed for a top event(gate)
with determined minimal cut sets or prime implicants [UA]_.
If events in the products have their probabilities
expressed by a statistical distribution with some uncertainties,
these uncertainties propagate to the total probability of the top event.
This analysis is performed employing the Monte Carlo Method.
The values of probabilities are sampled
to calculate the distribution of the total probability.


********************
Sensitivity Analysis
********************

Sensitivity analysis determines
how much the variation of each event
contributes to the total uncertainty of the top event(gate) [SA]_.
There are many approaches for this analysis,
but in general, the analyst modifies the structure of the problem tree or input values
to observe changes in results.
Key assumptions and issues can be examined at this stage.
However, since this analysis follows the uncertainty analysis,
the sensitivity analysis may be expensive.


*******************
Importance Analysis
*******************

The importance of a component or event provides information
about its impact on the system.
This analysis is used to filter out components
that need most attention to reduce the overall risk.

.. note:: The following interpretations are valid only for coherent fault trees.


Birnbaum
========

This factor is also called Marginal Importance Factor(MIF).
This factor gives the increase in risk due to the failure of the component
by measuring the difference between failed-event and non-failed event systems.

.. math::

    MIF = P(System/event) - P(System/NOT event)


Critical Importance Factor
==========================

This factor is also called Criticality Factor
and takes into account the reliability of the component.

.. math::

    CIF = P(event) / P(System) * MIF


Fussel-Vesely
=============

This factor is also called Diagnosis Importance Factor(DIF).
The value provides information
about how much the component is contributing to the total risk.

.. math::

    DIF = P(event/System) = P(event) * P(System/event) / P(System)


Risk Achievement Worth
======================

This factor is also called Risk Increase Factor
and measures the increase in risk of the system
given that the component has already failed.
This factor indicates the importance of
maintaining the component at its current level of reliability.

.. math::

    RAW = P(System/event) / P(System)


Risk Reduction Worth
====================

This factor is also called Risk Decrease Factor
and indicates the maximum decrease in risk of the system
if the component never failed or increased its reliability.
This factor helps select the components
to improve first with most effect on risk reduction.

.. math::

    RRW = P(System) / P(System/NOT event)


***************************
Incorporation of Alignments
***************************

The system's configuration may change over time due to
maintenance or substitutions of failed/out-of-service events.
This temporary configurations create different analyses and final results.


***************************
Dynamic Fault Tree Analysis
***************************

This analysis takes into account the order of events' failures.
The information about time dependency is incorporated into a fault tree
by using specific gates, such as Priority AND, Sequence.


GATES
=====

#. Priority AND (PAND) :
    Output fault occurs
    if all the input faults occur in a specific sequence.
    The sequence may also be from first to last member or left to right.
    In most packages with static fault tree analysis,
    this gate is treated just like AND gate without the sequence,
    so it stays for graphical purposes only.

#. Functional Dependency (FDEP) :
    This is not a gate with an output
    but a description that a set of basic events depends on one trigger event.
    If the trigger event occurs,
    all the basic events occur immediately and simultaneously (no ordering).
    To achieve this behavior with existing static gates,
    each occurrence of a basic event in the set
    can be replaced with an OR gate with two inputs,
    the basic event and the trigger.

#. Sequence Enforcer (SEQ) :
    This is not a gate with an output
    but a constraint that events can only occur in given order.

#. Spare Gates :
    A collection of spare parts
    ready to replace failed components.
    If there are no more replacements,
    the gate fails.
    The spare components can be shared and have a waiting state (hot, warm, cold).
    For simple analysis with hot spare components
    (the same failure characteristics as the deployed component),
    this gate can be approximated with an AND gate.


*************************
Reliability Block Diagram
*************************

RBD or Dependence Diagram(DD) is another way of showing the system component layout
using a diagram with series and parallel configurations [RBD]_.
In this analysis,
the success of the system is shown through the paths
that are still available after failure of a component.
That is, parallel paths are redundancies in the system.
The diagram can be converted to a success tree or fault tree.
More complex dependent relationships can be handled by a dynamic RBD.
