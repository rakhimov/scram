.. _fta_algorithms:

###############
FTA: Algorithms
###############

********
Overview
********

There are many algorithms developed for fault tree analysis since the 1960s,
but MOCUS-based and BDD-based algorithms are most used in current PRA software
[Rau93]_ [Rau03]_.
More information can be found in :ref:`references`.


MOCUS
=====

MOCUS is a top-down algorithm first proposed by J.Fussel and W.Vesely in 1972.
There are many suggested improvement techniques for this algorithm,
but the basics are simpler than for other FTA algorithms [Rau03]_.
In addition, MOCUS-like algorithms may incorporate approximations
to speed up their calculations;
however, this may lead to inaccuracies in overall analysis.
There seem not to be overall consensus on these approximations
over all the tools that use this algorithm.
The MOCUS algorithm is used by many FTA tools, such as RiskSpectrum and SAPHIRE.


Binary Decision Diagram
=======================

The BDD-based algorithm can use
various types of the Binary Decision Diagram ([BDD]_, [ZBDD]_)
for Boolean operations [Bry86]_ [Min93]_ [Rau93]_.
This is a bottom-up approach that is mature and well tuned for PRA
and other applications like electronics.
This method consists of many complex algorithms of the BDD to find MCS [Rau01]_.
The BDD algorithms tend to be faster than MOCUS and other algorithms;
however, this algorithm is subject to combinatorial explosion
for very large models with thousands of basic events and gates
with many replicated events.
One more important advantage of the BDD-based analysis is
that the BDD allows fast and exact calculation of probabilities,
which makes Probability, Importance, Uncertainty analyses fast as well [DR01]_.
This algorithm is used in CAFTA, RiskA, and RiskMan.


*************************************
Prime Implicants vs. Minimal Cut Sets
*************************************

Minimal cut sets are sets of basic events (variables)
that induce the failure of the top event [Rau01]_.
The set represents a conjunction of Boolean variables (product).
Another way to represent the minimal sets of basic events
is to consider the shortest paths for top event failure.
If one path includes another,
it is not the shortest
so is not minimal.

The notion of minimal cut sets with occurrence or failure of basic events
is appropriate for coherent fault trees.
For non-coherent fault trees with complements of basic events,
success or non-occurrence of basic events
may lead to the failure of the top event.
In order to capture these scenarios,
prime implicants are computed as products
that include complements of variables.

However, the notion of prime implicants may not be
as intuitive as the notion of minimal cut sets for analysts
since consideration of success of an undesired event
complicates failure scenarios
or may be irrelevant at all
if the probability of success is close to 1.

Minimal cut sets can be used as a conservative, approximate result
for analysis of non-coherent fault trees.
In order to eliminate complements of variables,
it is assumed that a complement of an event always occurs, i.e., constant True or 1,
unless the complement is in the same path or set as the corresponding event.
In the latter case, the path or set is Null or an impossible scenario
because an event and its complement are mutually exclusive.

This approximation is conservative
because the worst case scenario is considered for complements.
Moreover, the computation of minimal cut sets
is simpler than the computation of prime implicants.
Calculation of prime implicants involves
extra computations in accordance to the Consensus theorem
and generates a lot more products than calculation of minimal cut sets [PA09]_.

The benefit of calculation of prime implicants is
that it is the exact representation of the system without approximations.
Computation of prime implicants is necessary
if complements or success of events cannot or should not be ignored.
Relevancy notion is given to events
depending on the membership in products.
If an event is in a product as a positive literal,
the event is failure relevant.
If an event is in a product as a negative literal (complement of the event),
the event is repair (success) relevant.
If an event is not in a product,
it is irrelevant.

Given Boolean formula :math:`f(a,b,c)`:

    .. math::

        f(a,b,c) = a \& b \| \overline{a} \& c

Considering the complement is always True, the formula is simplified:

    .. math::

        f(a,b,c) = a \& b \| c

Computation of prime implicants requires computation of the consensus
when variable :math:`a` is irrelevant:

    .. math::

        f(a,b,c) = a \& b \| \overline{a} \& c \| b \& c

Minimal cut sets of the formula are :math:`{ab, c}`.
Prime implicants are :math:`{ab, ~ac, bc}`.


********************
SCRAM FTA Algorithms
********************

MOCUS
=====

This algorithm is similar to the MOCUS algorithm as described in [Rau03]_.
Steps in the algorithm for minimal cut set generation from a fault tree.

**Rule 1.** Each OR gate generates new rows(sets) in the table(set) of cut sets

**Rule 2.** Each AND gate generates new columns(elements) in the table(set) of cut sets

After finishing or each of the above steps:

**Rule 3.** Eliminate redundant elements that appear multiple times in a cut set

**Rule 4.** Eliminate non-minimal cut sets

The implementation uses a ZBDD data structure (a set of sets)
to store minimal cut sets in a compact form,
so Rules 3 and 4 are satisfied automatically.
Elements in a set have AND relationship with each other;
whereas sets in the ZBDD have OR relationship with each other.

Gates of top and intermediate events affect cut sets.
Each OR gate adds new sets into the set of sets,
while each AND gate adds additional elements
into one specific set or group of sets inside the set of sets.

To generate all cut sets,
the fault tree is traversed from the top to basic events,
In this step, the analysis may cancel cut sets
if the fault tree is non-coherent and contains complements.
In addition,
if a cut set order is larger than the limit put by a user,
it is discarded.

After required cut sets are generated,
the minimization of the generated cut sets
and extraction of minimal cut sets into a final (result) form
is delegated to ZBDD facilities.


Binary Decision Diagram
=======================

Binary decision diagrams are constructed from Boolean graphs for analysis.
In order to calculate minimal cut sets or prime implicants,
BDD is converted into Zero-suppressed binary decision diagrams (ZBDD).
ZBDD is a data structure that encodes sets in a compact way [Min93]_.
Minimization of sets is performed with subsume operations described in [Rau93]_.
After these operations,
any path leading to 1 (True) terminal
is extracted as a product.


Zero-Suppressed Binary Decision Diagram
=======================================

In addition to being a helpful facility for set minimization,
ZBDDs can work directly with Boolean graphs [Jun09]_.
The major benefit of this approach
is that products can be kept minimal and truncated upon generation.
However, the application of Boolean operators on the ZBDD decomposition
requires extra computations compared to the BDD approach.


********************
UNITY and NULL Cases
********************

The analyzed products may result in NULL(empty) or UNITY(base) sets,
which may indicate guaranteed success or failure.
These cases are handled as special
and given appropriate messages and probabilities.
UNITY(base) set shows only one empty product of order 1 and probability 1.
NULL(empty) set has probability 0 and shows no products.
