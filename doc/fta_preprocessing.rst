.. _preprocessing:

#############################
FTA: Preprocessing Techniques
#############################

In order to optimize the analysis and deal with the exponential complexity,
fault tree preprocessing is attempted before initiating other
Minimal Cut Set algorithms. There are many proposed optimizations; however,
the ordering and gain of their application may be unclear. Successful
preprocessing helps reduce the complexity substantially for large trees.


Constant Propagation
====================

House or external events are treated as Boolean constants and propagated
according to the Boolean algebra before other more complex and expensive steps.
This procedure prunes the fault tree. Null and unity branches are removed
from the tree leaving only basic events and gates.


Gate Normalization
==================

Initially, the fault tree is simplified to contain only AND and OR gates by
replacing complex gates like ATLEAST and XOR.


Propagating Complement
======================

Complements or negations are pushed down according to the De Morgan's law.


Gate Coalescing
===============
Gates of the same type or logic are coalesced according to the rules of Boolean
algebra. For example, AND gate parent and AND gate child are joined into a new
gate or added to the parent gate. This operation attempts to reduce the number
of gates to expand later. However, this operation may complicate the other
preprocessing steps to find modules or propagate failure in the tree.


Module Detection
================

Modules are defined as gates or group of events that do not have common events
or modules with other gates. Modules are analyzed as separate and independent
fault trees. If a module appears in the final minimal cut sets, then the cut
sets are populated with the minimal cut sets of the module. This operation
guarantees that the final, joint cut sets are minimal, and no expensive check
for minimality is needed. However, most complex fault trees do not contain big
modules in their original Boolean formula.


Formula Rewriting
=================

In order to simplify the Boolean formula of a fault tree and group independent
subtrees, formula rewriting can be attempted. One of the goals of this
procedure is to reduce the number of common events and gates. This procedure
may also help isolate those events and gates that appear in multiple places in
a fault tree, making the modularization process more effective.
