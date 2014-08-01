###################################
Fault Tree Generator Python Script
###################################

A description of python script to generate a tree of any size and complexity.

General Description
===================
* Use random numbers to determine the structure of the tree.
* Number of primary events is specified by a user.
* The seed of the random number generator may be fixed and specified as
  well.
* Names of events in the tree must be randomly chosen between creating a
  new event or re-using already created event.
* Probabilities for primary events are generated randomly.
* Names are assigned sequentially. E# and P#.
* The tree is deterministic upon setting the same parameters.
* The exact ratios are not guaranteed.
* No support for transfer ins or outs is expected.
* No support for CCF is expected.
* The output should be an input tree file and a probability file.


Script arguments
=================
* Random number generator seed.
* Number of primary events.
* Approximate ratio primary events to intermediate events. Average.
* Approximate ratio of reused primary events. This events may show up
  in several places in the tree.
* Minimum and maximum probabilities for primary events.
* Number of primary events for the root node of the tree.
* Fixed number of children for the root node of the tree.
* Output file name.
* Optional use of more complex gates and primary event types.

.. warning::
    Most values for the script arguments are not tested for validity. The
    output tree will be validated by SCRAM.
    For some invalid values the python script may scream itself.


Algorithm
==========

1) Generate databases with intermediate and basic events.

    * Top event do not have children primary events, by default,
      for complexity of the tree. The number of primary events for
      the root node may be set by a user.
    * Random choice between creating a new intermediate event,
      a new primary event, or re-using an existing primary event.
    * Limiting factors: the average number of primary events per intermediate
      event, and the total number of primary events.

2) Write the tree description into an output tree file.

3) Generate probabilities.

4) Write probabilities into a probability file.
