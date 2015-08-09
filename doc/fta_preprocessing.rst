.. _preprocessing:

#############################
FTA: Preprocessing Techniques
#############################

In order to optimize the analysis
and deal with the exponential complexity,
fault tree preprocessing is attempted
before initiating other fault tree analysis algorithms.
There are many proposed optimizations;
however, the ordering and gain of their application may be unclear.
Successful preprocessing helps reduce the complexity substantially for large trees.


Constant Propagation
====================

House or external events are treated as Boolean constants
and propagated according to the Boolean algebra
before other more complex and expensive preprocessing steps.
This procedure prunes the fault tree.
Null and unity branches are removed from the fault tree
leaving only basic events and gates.


Gate Normalization
==================

The fault tree is simplified to contain only AND and OR gates
by replacing complex gates like ATLEAST and XOR with AND and OR gates.


Propagating Complement
======================

Complements or negations of gates are pushed down to basic events
according to the De Morgan's law.


Gate Coalescing
===============

Gates of the same type or logic are coalesced
according to the rules of the Boolean algebra.
For example,
AND gate parent and AND gate child are joined into a new gate,
or the arguments of the child are added to the parent gate.
This operation attempts to reduce the number of gates to expand later.
However, this operation may complicate other preprocessing steps
that may try to find modules or propagate failure in the fault tree.


Module Detection
================

Modules are defined as gates or group of events
which sub-graph does not have common nodes with the rest of the graph.
Modules are analyzed as separate and independent fault trees.
If a module appears in the final minimal cut sets,
then the cut sets are populated with the minimal cut sets of the module.
This operation guarantees
that the final, joint cut sets are minimal,
and no expensive check for minimality is needed.
However, most complex fault trees do not contain big modules in their original Boolean formula.


Formula Rewriting
=================

In order to simplify the Boolean formula of a fault tree and group independent subtrees,
formula rewriting can be attempted.
One of the goals of this procedure is to reduce the number of common events and gates.
This procedure may also help isolate those events and gates
that appear in multiple places in a fault tree,
making the modularization process more effective.
