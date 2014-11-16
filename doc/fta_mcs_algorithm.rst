.. _mcs_algorithm:

###################################
SCRAM FTA: Finding Minimal Cut Sets
###################################

Fault Tree Constant Propagation
===============================
House or external events are treated as Boolean constants and propagated
according to the Boolean algebra before other more complex and expensive step.
This procedure prunes the fault tree. Null and unity branches are removed
from the tree leaving only basic events.

Fault Tree Preprocessing
========================
In order to optimize the analysis and deal with the exponential complexity,
fault tree preprocessing is attempted before initiating other
Minimal Cut Set algorithms. There are many proposed optimizations; however,
the ordering and gain of their application may be unclear.
Successful preprocessing help reduce the complexity
substantially for large trees.

Initially, the fault tree is simplified to contain only AND and OR gates by
replacing complex gates like ATLEAST and propagating complements. Then,
gates of the same type are coalesced according to the rules of Boolean algebra.
For example, AND gate parent and
AND gate child are joined into a new gate or added to the parent gate. This
operation attempts to reduce the number of gates to expand later. However,
this operation may complicate the further step to find modules in the tree.

The second operation is to find modules (sub-trees) of the tree.
Modules are defined as gates or group of events that do not share events or
modules with other gates. Modules are analyzed as a separate and
independent fault trees. If a module appears in the final minimal cut sets,
then the cut sets are populated with the minimal cut sets of the module.
This operation guarantees that the final joint cut sets are minimal, and no
expensive check for minimality is needed. However, most complex fault trees
do not contain big modules in their original Boolean formula.

In order to simplify the Boolean formula of a fault tree and group independent
subtrees, formula rewriting can be attempted. One of the goals of this
procedure is to reduce the number of shared events and gates. This procedure
may also help to isolate those events and gates that appear in multiple
places in a fault tree, making the modularization process more effective.


Minimal Cut Set (MCS) Generation Algorithm
===========================================
Steps in the algorithm for minimal cut set generation from a fault tree.
This algorithm is similar to the algebraic method or MOCUS expansion.

**Rule 1.** Each OR gate generates new rows(sets) in the table(set) of cut sets

**Rule 2.** Each AND gate generates new columns(elements) in the table(set) of cut sets

After finishing or each of the above steps:

**Rule 3.** Eliminate redundant elements that appear multiple times in a cut set

**Rule 4.** Eliminate non-minimal cut sets

MCS Generation Implementation Specifics
========================================
The implemented algorithm is similar to the MOCUS algorithm, but it is not
yet formally verified as a MOCUS-like algorithm.
The actual implementation uses a set of sets in order to avoid duplicates,
so Rule 3 is satisfied automatically. Elements in a set have AND relationship
with each other; whereas sets in a set of sets have OR relationship with
each other. Therefore, sets of elements represent cut sets.

Gates of intermediate events affect cut sets. Each OR gate adds new sets into
the set of sets, while each AND gate adds additional elements into one
specific set inside the set of sets.

In the first pass, the fault tree is traversed from the top to down, and all
cut sets are generated. In this step, the analysis may cancel cut sets if
the fault tree is non-coherent and contains NOT elements. Also, if a cut set
is larger than the limit set by a user, it is discarded.

After the first pass, all possible and required cut sets are generated.
The generated cut sets are stored in a set. An attempt to reduce the number
of generated cut sets is made by detecting cut set minimality earlier
in the fault tree traversal. To accomplish this, the basics of Boolean algebra
are taken into account upon traversing gates.

The next algorithmically complex part is the application of Rule 4 or
minimization of the generated cut sets. The set of cut sets is traversed to
find sets with size one, which are minimal by default.
Then all other sets are checked if they are supersets of the found
minimal sets so far. If this is not the case, the sets with size 2 are
minimal, and these size 2 cut sets are added into the minimal cut sets.
Then these size 2 minimal cut sets are used to check if they are subsets of
the other cut sets. If not, then size 3 cut sets are minimal.
This incremental logic is continued till the initial set of cut sets is empty.

Proposed Improvements for this algorithm
----------------------------------------

- Use of Binary Decision Diagrams for efficient MCS detection.

Other MCS Generation Algorithms
===============================
According to :ref:`papers` about probabilistic safety and risk analysis,
there are many algorithms developed for fault tree analysis since the 1960s,
but MOCUS-based and BDD-based algorithms are most used in current PRA software.

The first algorithm is MOCUS, a top-down algorithm first proposed by J.Fussel
and W.Vesely in 1972. There are many suggested improvement techniques for
this algorithm, but the basics are simpler than for other FTA algorithms.
In addition, MOCUS-like algorithms may incorporate approximations to speed
up their calculations; however, this may lead to inaccuracies in overall
analysis. There seem not to be overall consensus on these approximations
over all the tools that use this algorithm.
The MOCUS algorithm is used by many FTA tools, such as RiskSpectrum and
SAPHIRE.

The second algorithm uses various types of the Binary Decision Diagram (BDD)
for Boolean operations. This is a bottom-up algorithm that is
mature and well tuned for PRA and other applications like electronics.
This method consists of many complex algorithms of BDD to find MCS.
The BDD algorithms tend to be faster than MOCUS and other algorithms; however,
this algorithm is subject to combinatorial explosion for very large models with
thousands of basic events and gates with many replicated events. One more
important advantage of BDD based analysis is that BDD allows fast and exact
calculation of probabilities, which makes Probability, Importance,
Uncertainty analyses fast as well.
This algorithm is used in CAFTA, RiskA, and RiskMan.
