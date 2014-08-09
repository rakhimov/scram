#############################################
SCRAM FTA: Minimal Cut Sets
#############################################

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
