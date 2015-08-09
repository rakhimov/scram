###################
Fault Tree Graphing
###################

In order to make visual verification of a fault tree,
other diagram and graphing packages are employed.
SCRAM writes a file with instructions for the external graphing tools.


SCRAM Fault Tree Graphing Specifics
===================================

#. Called after the tree initialization steps.
#. Probability information is optional.
#. Gate repetition is graphed with transfer symbols.
#. Assigns colors for clarity:

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

- `Graphviz DOT`_ (*Prefer SVG output for complex trees*)

.. _Graphviz DOT: http://www.graphviz.org


.. note:: One file per top event is produced with the fault tree and top event names.


Example Fault Tree Images
=========================

Two Train System
----------------

.. image:: two_train.png


Lift
-----------

.. image:: lift.png
