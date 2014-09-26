.. _mcs_algorithm:

###########################
SCRAM FTA: Minimal Cut Sets
###########################

Fault Tree Preprocessing
========================
In order to optimize the analysis, fault tree preprocessing is attempted
before initiating other Minimal Cut Set algorithms. Initially, the gates are
merged according to rules of Boolean algebra. For example, AND gate parent and
AND gate child are joined into a new gate or added to the parent gate. This
operation attempts to reduce the number of gates to expand later.

The second operation is to find modules of the tree. Modules are defined as
gates or group of events that do not share events or modules with other gates.
Modules are expanded to get the number of primary events of the module. If
the module appears in the final minimal cut sets, then the cut sets are
populated with the event of the module. [not implemented]


Minimal Cut Set (MCS) Generation Algorithm
===========================================
Steps in the Algorithm for generation of minimal cut sets of a fault tree.
This algorithm is similar to the algebraic method.

**Rule 1.** Each OR gate generates new rows in the table of cut sets

**Rule 2.** Each AND gate generates new columns in the table of cut sets

After each of the above steps:

**Rule 3.** Eliminate redundant elements that appear multiple times in a cut set

**Rule 4.** Eliminate non-minimal cut sets

MCS Generation Implementation Specifics
==========================================
The implemented algorithm is similar to the MOCUS algorithm, but it is not
yet formally verified as the MOCUS-like algorithm.
The actual implementation uses a set of sets in order to avoid duplicates,
so Rule 3 is satisfied automatically. Elements in a set have AND relationship
with each other; whereas sets in a set of sets have OR relationship with
each other. Therefore, sets of elements represent cut sets.

A special Superset class is implemented to deal with various types of
events in the set. It separates intermediate and primary events.

Gates of intermediate events affect cut sets. Each OR gate adds new sets into
the set of sets, while each AND gate adds additional elements into one
specific set inside the set of sets.

In the first pass, the fault tree is traversed from the top to down, and all
cut sets are generated. In this step, Superset class may cancel cut sets if
the fault tree is non-coherent and contains NOT elements. Also, if a cut set
is larger than the limit set by a user, it is discarded.

After the first pass, all possible and required cut sets are generated.
The generated cut sets are stored in a set. The next algorithmically complex
part is the application of Rule 4. The set of cut sets is traversed to find
sets with size one, which are minimal by default.
Then all other sets are checked if they are supersets of the found
minimal sets so far. If this is not the case, the sets with size 2 are
minimal, and these size 2 cut sets are added into minimal cut set collection.
Then these size 2 minimal cut sets are used to check if they are subsets of
the other cut sets. If not, then size 3 cut sets are minimal.
This incremental logic is continued till the initial set of cut sets is empty.

Proposed Improvements for this algorithm
------------------------------------------

Other MCS Generation Algorithms
===============================
According to :ref:`papers` about probabilistic safety and risk analysis,
there are two main algorithms for PRA software.

The first algorithm is MOCUS, a top-down algorithm first proposed by J.Fussel
and W.Vesely in 1972. There are many suggested improvement techniques for
this algorithm, but the basics are simpler than for other FTA algorithms.
In addition, MOCUS-like algorithms may incorporate approximations to speed
up their calculations; however, this may lead to inaccuracies in overall
analysis. There seem not to be overall consensus on these approximations
over all the tools that use this algorithm.
The MOCUS algorithm is used by many FTA tools, such as RiskSpectrum,
SAPHIRE and XFTA.

The second algorithm uses various types of the Binary Decision Diagram (BDD)
for Boolean operations. This is a bottom-up algorithm and seems to be
mature and well tuned for PRA and other applications like electronics.
This method consists of many complex algorithms of BDD to find MCS.
The BDD algorithms tend to be faster than MOCUS and other algorithms; however,
this algorithm is subject to combinatorial explosion for very large models with
thousands of basic events and gates with many replicated events.
This algorithm is used in CAFTA and RiskMan.
