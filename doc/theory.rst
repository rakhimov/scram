######
Theory
######

This section contains short descriptions of the analyses used in
probabilistic risk assessment. More information can be found in :ref:`papers`.

Static Fault Tree Analysis
==========================

Fault Tree Analysis is a top-down deductive method to understand the logic
leading to the top undesired event or fault. Boolean logic is used to combine
events that could lead to the undesired top event. This analysis assumes that
events are independent and may incorporate Common Cause Failure for the
analysis with dependent systems. The analysis generates minimal cut sets,
importances of events, probabilities of top event and gates.

Static Fault Tree Symbols, Gate and Event Types
===============================================
EVENTS
------
#. Top Event :
    Contains a description of the top-system-level fault,
    or undesired event. The root of a fault tree. Top event only
    receives input from a logic gate.

#. Intermediate/Fault Event :
    Contains a description of a lower-level
    fault in the system. Intermediate events receive input
    from and provide outputs to a logic gate.

#. Basic Event :
    Contains a failure at the lowest level of examination, which
    has a potential to cause a fault in the system. This is a
    leaf of a fault tree, and it provides input to a logic gate.
    This event must have its probability defined.

#. Undeveloped Event :
    Similar to a basic event, but an undeveloped event has
    its own causes that may be analyzed by a separate fault tree.
    This event is not further developed either because of
    insufficient consequence or because information is unavailable.
    This event must have its probability defined and provide input
    to a logic gate.

#. House/Input/External Event :
    Contains a normal system operating input with
    the capability of causing a fault. This event is expected to
    occur with probability 0 or 1 only, or it is sometimes used
    as a switch of True or False. Provides input to a logic gate.
    May be used to switch off a sub-tree.

#. Conditional Event :
    Defines a specific condition or restriction
    that apply to any logic gate. Used mostly with Priority AND and
    Inhibit gates. The alternative name is a Conditional Qualifier.
    Conditions may be abnormal cases or environment. The probability
    of this event might be a conditional probability.

SYMBOLS
-------
#. Transfer In :
    This symbol indicates that the tree is expanded further at
    the occurrence of the corresponding Transfer Out symbol.
    This expansion may be described in another file or page.

#. Transfer Out :
    This symbol indicates that this portion of the tree must be
    attached at the corresponding Transfer In symbol of the main
    fault tree.

GATES
-----
#. AND :
    Output fault occurs if all of the input faults occur.

#. OR :
    Output fault occurs if at least one of the input faults occur.

#. Priority AND :
    Output fault occurs if all of the input faults occur in a
    specific sequence. This gate is for the dynamic tree analysis.
    The sequence may also be from first to last member or left to right.
    For most packages with the static fault tree analysis, this gate is
    treated just like AND gate without the sequence, so it stays for
    graphical purposes only.

#. Exclusive OR (XOR) :
    Output fault occurs if exactly one of the two input
    faults occurs. (A XOR B) = (A AND B') OR (A' AND B). In other words,
    the result is one or the other but not both.
    This gate can only have two inputs. For several inputs,
    the output fault occurs if odd number of faults occur. This can be
    simulated by chaining XOR gates if needed.

#. Inhibit :
    This gate is a special case of the AND gate.
    Output fault occurs if the single input fault occurs in the
    presence of an enabling condition (the enabling condition is
    represented by a Conditioning Event drawn to the right of the
    gate.). This gate restricts input events to only two events.

#. Combination/VOTE/Atleast :
    Output fault occurs if **m** out of the **n** input events
    occurs. The **m** input events need not to occur simultaneously. The output
    occurs if at least **m** events occur. **m** is more than 1, and **n**
    is more than **n**.

#. NOT :
    Output fault occurs if the input event does not occur.
    This logic leads to non-coherent trees, for which non-occurrence or success
    of events may lead to occurrence of the main undesirable event.

#. NULL/Pass-Through :
    Non-essential. Only one input is allowed.
    Used in applications with GUI to allow more description or alignment.

#. NAND :
    NOT AND gate. Indicates that the output occurs when at least one
    of the input events is absent. This may lead to non-coherent
    trees, where non-occurrence of an event causes the top event.

#. NOR :
    NOT OR gate. Indicates that the output occurs when all the input
    events are absent. This may also lead to non-coherent trees.

OTHER
-----
#. Dormant Failure :
    Failures that are not detected by themselves and need
    secondary specific actions or failures to occur.
    This is a special case of a primary event that may fail with
    no visible external effects.
    May be treated as a basic event for primitive analysis.


Event Tree Analysis
===================

Event Tree Analysis is a bottom-up approach to quantify the risk resulting
from an initiating event. The tree is branched into conditionally independent,
mutually exclusive cases, which lead to several final scenarios, outcomes, or
end states.
This analysis is conceptually useful when the system incorporates
sequentially occurring events.
Most of the time, there are two branches for success and failure cases, but
there may be more as long as the events are mutually exclusive.
Probabilities of intermediate cases can be calculated with fault trees or
assigned manually, and they must sum to 1 for mutually exclusive
and independent branches.

Fault Tree Linking
------------------

If the original assumption of independent branches does not hold, an event
tree branches can be linked to corresponding gates in fault trees, and the
final tree analyzed as a big fault tree.


Dynamic Fault Tree Analysis
===========================

This analysis takes into account the order of events' failures. The information
about time dependency is incorporated into a fault tree by using specific
gates, such as Priority AND, Sequence.

Common Cause Failure
====================

If events are not statistically independent, common cause or mode analysis is
performed to account for the failure of multiple elements at the same time or
within a short time. These common mode failures may be due to the same
manufacture flaws and design, environment, working conditions, maintenance,
quality control, normal wear and tear, and many other factors.
Several models are used to quantify the common cause failures.

Multiple Greek Letter(MGL) System
---------------------------------

Alpha System
------------

Beta System
-----------

Phi System
----------


Uncertainty Analysis
====================

Uncertainty quantification is performed for top event(gate) with determined
minimal cut sets. If events in the minimal cut sets have their probabilities
expressed by a statistical distribution with some uncertainties, these
uncertainties propagate to the total probability of the top event. This
analysis is performed employing the Monte Carlo Method. The values of
probabilities are sampled to calculated the distribution of the total
probability.

Sensitivity Analysis
====================

Sensitivity analysis determines how much the variation of each event
contributes to the total uncertainty of the top event(gate).
There are many approaches for this analysis, but in general, the analyst
modifies the structure of the problem tree or input values to observe
changes in results. Key assumptions and issues can be examined at this stage.
However, since this analysis follows the uncertainty analysis,
the sensitivity analysis may be expensive.

Importance Analysis
===================

Fussel-Vesely
-------------
This factor is also called Diagnosis Importance Factor.

Birnbaum
--------
This factor is also called Marginal Importance Factor.

Critical Importance Factor
--------------------------

Risk Reduction Worth
--------------------
This factor is also called Risk Decrease Factor.

Risk Achievement Worth
----------------------
This factor is also called Risk Increase Factor.

Incorporation of Alignments
===========================

Reliability Block Diagram
=========================
