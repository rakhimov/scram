############################################
SCRAM Fault Tree Graphing
############################################

In order to make visual verification of a created tree, other diagram and
graphing packages are employed. SCRAM writes a file with instructions for
these external graphing tools.

SCRAM Graphing Specifics
========================
#. Should be called after tree initiation steps.
#. May operate without probabilities.
#. Does not include transfer sub-trees.
#. May include descriptions of events. *Not Supported Yet*
#. May incorporate compact and full trees. Gates only vs. Event descriptions.
   *Not Supported Yet*
#. ISO standard shapes for gates and events. *Not Supported Yet*

Currently Supported Graphing Tools
==================================
* `Graphviz DOT`_

.. _`Graphviz DOT`: http://www.graphviz.org
