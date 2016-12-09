.. _preprocessing:

#############################
FTA: Preprocessing Techniques
#############################

In order to optimize the analysis
and deal with the exponential complexity,
fault tree preprocessing is attempted
before initiating other fault tree analysis algorithms.

The preprocessing algorithms deal with
transformations of propositional directed acyclic graphs ([PDAG]_)
to reduce the complexity and divide-and-conquer the problem.
There are many proposed algorithms,
and successful application of preprocessing techniques helps reduce substantially
the complexity of analysis of large graphs.
However, the ordering of preprocessing algorithms is not always clear
due to their side-effects,
and the performance gain is not guaranteed.
Some preprocessing techniques may only work
for certain structures or particular setups in the graph.


Constant Propagation
====================

House or external events are treated as Boolean constants
and propagated according to the Boolean logic
before other more complex and expensive preprocessing steps [NR99]_ [Rau03]_.
This procedure prunes the fault tree.
Null and unity branches are removed from the fault tree
leaving only basic events and gates.


Gate Normalization
==================

The fault tree is simplified to contain only *AND* and *OR* gates
by rewriting complex gates like *VOTE* and *XOR* with *AND* and *OR* gates
[Nie94]_ [Rau03]_.
After this operation,
the graph is in normal form.


Complement Propagation
======================

Complements or negations of gates are pushed down to basic events
according to the De Morgan's law [Rau03]_.
This procedure transforms the graph into negation normal form ([NNF]_)
if the graph is normal before the propagation.


Gate Coalescing
===============

Gates of the same type or logic are coalesced
according to the rules of the Boolean algebra [Nie94]_ [Rau03]_.
For example,
*AND* gate parent and *AND* gate child are joined into a new gate,
or the arguments of the child are added to the parent gate.
This operation attempts to reduce the number of gates to expand later.
However, this operation may complicate other preprocessing steps
that may try to find modules or propagate failure in the fault tree.


Module Detection
================

Modules are defined as gates or group of nodes
whose sub-graph does not have common nodes with the rest of the graph.
Modules are detected and analyzed
as separate and independent fault trees [DR96]_.
If a module appears in the final products,
then the products are populated with the sum of products of the module.
This operation guarantees
that the final, joint sum is minimal,
and no expensive check for minimality is needed.
However, most complex fault trees do not contain big modules in their original Boolean formula.


Multiple Definition Detection
=============================

Gates with the same logic and arguments
can be considered to be multiply defined.
Any gate from this group of multiply defined gates
can represent all of them in the graph,
reducing the size of the graph [Nie94]_ [NR99]_.
The result of this preprocessing technique
can help other preprocessing algorithms
that work with common nodes or
detect independent sub-graphs.


The Shannon Decomposition for Common Nodes
==========================================

Application of the Shannon decomposition for particular setups
with an AND/OR gate with common descendant nodes in the gate's sub-graph.

    .. math::

        x \& f(x, y) = x \& f(1, y)

        x \| f(x, y) = x \| f(0, y)

This technique is also called Constant Propagation [NR99]_ [Rau03]_,
but the actual constant propagation is only the last part of the procedure;
though, it is the main benefit of this preprocessing technique.
Some ancestors of the common node in the sub-graph
may need to be cloned,
which increases the size of the graph,
if the ancestors are common nodes themselves
and linked to other parts of the whole graph.
The application of this technique may be limited
due to performance and memory considerations
for complex graphs with many common nodes.


Distributivity Detection
========================

This is a formula rewriting technique
that detects common arguments in particular setups
corresponding to the distributivity of *AND* and *OR* operators [Nie94]_.

    .. math::

        (x \| y) \& (x \| z) = x \| (y \& z)

        (x \& y) \| (x \& z) = x \& (y \| z)

This technique helps reduce the number of common nodes;
however, it gets trickier to find the most optimal rewriting
with more complex setups
where more than one rewriting is possible.

    .. math::

        (x \| y) \& (x \| z) \& (y \| z)


Merging Common Arguments
========================

Common arguments of gates with the same logic
can be merged into a new gate with the same logic as the group [NR99]_.
This new gate can replace the common arguments
in the set of arguments of gates in the group.
Successful application of this technique
helps reduce the complexity
of the BDD construction from the graph.
Moreover,
by reducing the number of common nodes,
this technique may help isolate the common nodes into modules.


Boolean Optimization
====================

This optimization technique
detects redundancies in the graph
by propagating failures of common nodes
and noting the failure destinations [Nie94]_.
The redundant occurrences of common nodes are minimized
by directly transferring the common node
and its failure logic to the failure destinations.

The generalization of this technique
comes from the observations
of special cases for the Shannon decomposition.
Given a Boolean formula :math:`f(x, y)`,
the following cases are the special cases of its Shannon decomposition:

    1. If ``f(x, y) = 1/True/Failure`` assuming ``x = 1/True/Failure``:

        .. math::

            f(x, y) = x \| f(0, y)

    2. If ``f(x, y) = 0/False/Success`` assuming ``x = 1/True/Failure``:

        .. math::

            f(x, y) = \overline{x} \& f(0, y)

    3. If ``f(x, y) = 1/True/Failure`` assuming ``x = 0/False/Success``:

        .. math::

            f(x, y) = \overline{x} \| f(1, y)

    4. If ``f(x, y) = 0/False/Success`` assuming ``x = 0/False/Success``:

        .. math::

            f(x, y) = x \& f(1, y)

There may be many setups
that satisfy these special cases in a Boolean graph,
but only few transformations are beneficial.
Transformations with disjunctions of the formula (cases 1 and 3)
are the most desirable for analysis
because the final result of the analysis is the disjunction of products.

The main optimization criterion for transformations
is to decrease the complexity or multiplicity of the graph.
That is, the transformation must yield
fewer destinations than its original multiplicity.
This kind of successful transformations
may help other preprocessing techniques
achieve better results with the simpler graph as well.
