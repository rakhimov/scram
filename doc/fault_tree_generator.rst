##################################
Fault Tree Generator Python Script
##################################

The complexity of a fault tree depends on many factors, such as types of gates,
the number of shared nodes, the number of nodes, and the structure of the tree
or the arrangement of the nodes.
It is best to craft a tree to test complex and most
demanding cases, but it requires good understanding of fault trees and
may be time consuming to design large trees.
In order to facilitate the creation of complex trees,
a python script is written that takes into account the factors that make
a fault tree complex for analysis.

General Description
===================
* Use of pseudo-random numbers to determine the structure of the tree.
* Number of basic events is specified by a user.
* The seed of the random number generator may be fixed and specified as
  well.
* Nodes in the tree are randomly chosen between creating a
  new event or re-using an already created event.
* Probabilities for basic events are generated randomly.
* Names are assigned sequentially. E#, H#, CCF#, and G#.
* The tree is reproducible with the same parameters and the seed.
* The exact ratios and expected results are not guaranteed except for the
  number of basic events.
* The output is topologically sorted and a valid input for analysis tools.

Script arguments
================
* Random number generator seed.
* Number of primary events.
* Number of house events.
* Number of CCF (MGL only) groups.
* Approximate ratio of basic events to gates.
* Approximate ratio of reused basic events. This events may show up
  in several places in the tree.
* Approximate ratio of reused gates. The acyclic property is ensured.
* Maximum number of children per gate. The average is around (Max+2)/2.
* Minimum and maximum probabilities for primary events.
* Number of basic events for the root node of the tree.
* Fixed number of children for the root node of the tree.
* Weights for the gate types: AND, OR, K/N, NOT, XOR.
* Optional use of more complex gates (K/N, NOT, XOR) only if the weights
  are given.
* Output file name.
* Output formats: shorthand or XML(default).

Note on Performance
===================
Depending on the provided arguments for the script, the execution time
varies greatly. The number of gates and their reuse in the fault tree
generation tend to greatly increase the execution time because of the need to
check for cycles. If the number of gates and their reuse are kept constant,
the generation time scales linearly with the number of basic events.

The average time complexity is approximately

    .. math::

        O(N*(N/Ratio)^2*ReuseG*\exp(ReuseP)*\exp(-MaxChildren/Ratio))

It is possible to generate a 100,000 basic event tree in less than a minute;
however, to generate more complex fault trees,
it is recommended to use Cython_.
Cython can convert the fault tree generator script into C code, which can be
compiled into a faster executable. This faster executable is capable of
generating million-basic-event fault trees in few minutes.

.. _Cython:
    http://cython.org/
