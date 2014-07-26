#######################################
SCRAM
#######################################

The overall description of SCRAM and possible future additions.
These descriptions may not reflect current capabilities.

Command-line Call of SCRAM by a User.
=====================================

Type of analysis, i.e., fault tree, even tree, Markov chain, etc.
Upon calling from command-line, the user should indicate which analysis
should be performed. In addition to this, the user should provide
additional parameters for the analysis type chosen. For example, if
FTA is requested, type of algorithm for minimal cut set generation may
be specified by the user, or the user can choose if probability
calculations should assume a rare event approximation.
However, there are default values for these parameters that try to
give the most optimal and accurate results.


Fault Tree Analysis.
====================

#. Reading input files. Verification of the input. Optional visual tool.

#. (*Optional*) Upon users' request output instruction file for graphing
   the tree. This is for visual verification of the input.
   Stop execution of the program.

#. Create the tree for analysis.

#. Perform cut set generation.

#. Perform analysis: Rare event approximation and independence of events.
   Rare event approximation is used only if enforced by a user.

#. Output the user specified analysis results. The output is sorted by
   the order of minimal cut sets, their probabilities. In addition,
   the contribution of each primary event is given in the output.


Fault Tree Symbols, Gate and Event Types.
=========================================
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

#. Combination/VOTE :
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


Event Tree Analysis.
====================
#. Reading input files. Verification of the input. Optional visual tool.
#. (*Optional*) Upon users' request output instruction file for **graphviz**
   dot to draw the tree. This is for visual verification of the input.
   Stop execution of the program.
#. Create the tree for analysis.
#. Perform calculations.
#. Output the results.


Future Additions.
=================
#. Simple event tree analysis.
#. More efficient algorithms for fault tree analysis.
#. More types of gates for fault trees: exclusive OR, priority AND, inhibit.
#. More types of events for fault trees: conditioning.
#. Monte Carlo Methods.
#. Markov analysis.
#. Success tree by inverting minimal cut sets into minimal path sets.


General Information for Users.
==============================

#. Suggested scram specific extensions for input files:
    :FTA input file:     .scramf
    :FTA prob file:      .scramp

#. If you are using text editor with highlighting, set filetype to 'conf'.
   This configuration like highlighting works well with scram syntax.

#. Run 'scram -h' to see all the flags and parameters for analysis.

#. The minimum cut set generation for a fault tree and probability calculations
   may use a lot of time and computing power.
   Adjust SCRAM flags and parameters to reduce these demands.
