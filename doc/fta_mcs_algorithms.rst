.. _mcs_algorithm:

#############################
FTA: Finding Minimal Cut Sets
#############################

Minimal Cut Set (MCS) Generation Algorithm
==========================================

Steps in the algorithm for minimal cut set generation from a fault tree.
This algorithm is similar to the MOCUS algorithm.

**Rule 1.** Each OR gate generates new rows(sets) in the table(set) of cut sets

**Rule 2.** Each AND gate generates new columns(elements) in the table(set) of cut sets

After finishing or each of the above steps:

**Rule 3.** Eliminate redundant elements that appear multiple times in a cut set

**Rule 4.** Eliminate non-minimal cut sets


MCS Generation Implementation Specifics
=======================================

The implemented algorithm is similar to the MOCUS algorithm,
but it is not formally verified as a MOCUS-like algorithm.
The actual implementation uses a set of sets in order to avoid duplicates,
so Rule 3 is satisfied automatically.
Elements in a set have AND relationship with each other;
whereas sets in a set of sets have OR relationship with each other.
Therefore, a set of elements represents a cut set.

Gates of top and intermediate events affect cut sets.
Each OR gate adds new sets into the set of sets,
while each AND gate adds additional elements into one specific set inside the set of sets.

To generate all cut sets,
the fault tree is traversed from the top to basic events,
In this step, the analysis may cancel cut sets
if the fault tree is non-coherent and contains complements.
In addition,
if a cut set size is larger than the limit put by a user,
it is discarded.

The generated cut sets are stored in a set.
The basics of Boolean algebra are taken into account upon the fault tree traversal
to detect cut set minimality earlier
and to reduce the number of generated cut sets.

After all possible and required cut sets are generated,
the next algorithmically complex part
is the application of Rule 4 or minimization of the generated cut sets.
The set of cut sets is traversed to find sets of size one,
which are minimal by default.
Then all other sets are tested
if they are supersets of the found minimal sets so far.
If this is not the case,
the sets with size 2 are minimal,
and these size 2 cut sets are added into the minimal cut sets.
Then these size 2 minimal cut sets are used to check
if they are subsets of the remaining cut sets.
If not, then size 3 cut sets are minimal.
This incremental logic is continued till the initial set of cut sets is empty.


UNITY and NULL Cases
--------------------

The analyzed minimal cut sets may result in NULL or UNITY sets,
which may indicate guaranteed success or failure.
These cases are handled as special
and given appropriate messages and probabilities.
For example,
UNITY set will show only one empty minimal cut set of order 0 but probability 1.


Proposed Improvements for this algorithm
----------------------------------------

- Use of Binary Decision Trees or Diagrams for faster MCS detection.


Other MCS Generation Algorithms
===============================

There are many algorithms developed for fault tree analysis since the 1960s,
but MOCUS-based and BDD-based algorithms are most used in current PRA software.

MOCUS is a top-down algorithm first proposed by J.Fussel and W.Vesely in 1972.
There are many suggested improvement techniques for this algorithm,
but the basics are simpler than for other FTA algorithms.
In addition, MOCUS-like algorithms may incorporate approximations
to speed up their calculations;
however, this may lead to inaccuracies in overall analysis.
There seem not to be overall consensus on these approximations
over all the tools that use this algorithm.
The MOCUS algorithm is used by many FTA tools, such as RiskSpectrum and SAPHIRE.

The BDD-based algorithm can use
various types of the Binary Decision Diagram (BDD) for Boolean operations.
This is a bottom-up approach that is mature and well tuned for PRA
and other applications like electronics.
This method consists of many complex algorithms of the BDD to find MCS.
The BDD algorithms tend to be faster than MOCUS and other algorithms;
however, this algorithm is subject to combinatorial explosion
for very large models with thousands of basic events and gates
with many replicated events.
One more important advantage of the BDD-based analysis is
that the BDD allows fast and exact calculation of probabilities,
which makes Probability, Importance, Uncertainty analyses fast as well.
This algorithm is used in CAFTA, RiskA, and RiskMan.

More information can be found in :ref:`papers`.
