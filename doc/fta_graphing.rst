############################################
SCRAM Fault Tree Graphing
############################################

In order to make visual verification of a created tree, other diagram and
graphing packages are employed. SCRAM writes a file with instructions for
these external graphing tools.

SCRAM Fault Tree Graphing Specifics
====================================
#. Should be called after tree initiation steps.
#. May operate without probabilities.
#. Does not include transfer sub-trees.
#. Graphes sub-trees in isolation.
#. One node reference to sub-tree cannot be graphed from a main tree file.
#. Tries to make the tree look compact.
#. Assign colors for clarity:
    1. Gates and Node Colors:

        :OR:          Blue
        :AND:         Green
        :NOT:         Red
        :XOR:         Brown
        :Inhibit:     Yellow
        :VOTE:        Cyan
        :NULL:        Gray
        :NOR:         Magenta
        :NAND:        Orange

    2. Primary Events and Text Colors:

        :Basic:             Black
        :Undeveloped:       Blue
        :House:             Green
        :Conditional:       Red


Currently Supported Graphing Tools
==================================
* `Graphviz DOT`_

.. _`Graphviz DOT`: http://www.graphviz.org
