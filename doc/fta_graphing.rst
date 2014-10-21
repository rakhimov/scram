#########################
SCRAM Fault Tree Graphing
#########################

In order to make visual verification of a created tree, other diagram and
graphing packages are employed. SCRAM writes a file with instructions for
these external graphing tools.

SCRAM Fault Tree Graphing Specifics
====================================
#. Should be called after tree initiation steps.
#. May operate without probabilities.
#. Gate repetition is graphed with a transfer symbol.
#. Tries to make the tree look compact.
#. Assign colors for clarity:
    1. Gates and Node Colors:

        :OR:          Blue
        :AND:         Green
        :NOT:         Red
        :XOR:         Brown
        :Inhibit:     Yellow
        :ATLEAST:     Cyan
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
* `Graphviz DOT`_ (*Prefer SVG output for complex trees*)

.. _`Graphviz DOT`: http://www.graphviz.org

Example Fault Tree Images
=========================
Two Train System
----------------
.. image:: two_train.png

Lift
-----------
.. image:: lift.png
