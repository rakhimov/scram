##################################
Fault Tree Generator Python Script
##################################

The complexity of a fault tree depends on many factors, such as types of gates,
the number of shared nodes, the number of nodes, and the structure or the
arrangement of the tree. It is best to craft a tree to test complex and most
demanding cases, but it requires good understanding of fault trees and
may be time consuming to design large trees.
In order to facilitate the creation of complex trees,
a python script is written that takes into account the factors that make
a fault tree complex for analysis. More features for this script will
be introduced as SCRAM becomes capable of handling more complex trees.

General Description
===================
* Use random numbers to determine the structure of the tree.
* Number of primary events is specified by a user.
* The seed of the random number generator may be fixed and specified as
  well.
* Names of events in the tree must be randomly chosen between creating a
  new event or re-using an already created event.
* Probabilities for primary events are generated randomly.
* Names are assigned sequentially. E# and G#.
* The tree is deterministic upon setting the same parameters.
* The exact ratios are not guaranteed.
* The output should be an input tree file.

Script arguments
=================
* Random number generator seed.
* Number of primary events.
* Approximate ratio primary events to gates. Average.
* Approximate ratio of re-used primary events. This events may show up
  in several places in the tree.
* Approximate ratio of re-used gates. The acyclic property should be ensured. [not implemented]
* Minimum and maximum probabilities for primary events.
* Number of primary events for the root node of the tree.
* Fixed number of children for the root node of the tree.
* Output file name.
* Optional use of more complex gates and primary event types. [not implemented]

Algorithm
==========

1) Generate databases with intermediate and basic events.

    * Top event do not have children primary events, by default,
      for the higher complexity of the tree. The number of primary events for
      the root node may be set by a user.
    * Random choice between creating a new intermediate or
      primary event, or re-using an existing primary or intermediate event.
    * Limiting factors: the average number of primary events per intermediate
      event, and the total number of primary events.

2) Generate probabilities.

3) Write the tree description into an output tree file.
