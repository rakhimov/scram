###########
Input Files
###########

SCRAM uses XML input files to describe analysis. See :ref:`xml_tools` for
more convenient writing and reading of XML input files.

Currently, only fault trees are accepted for analysis. The input file format
follows `OpenPSA Model Exchange Format v2.0d`_ or later. The extensive
description is given in the above format documentation by OpenPSA,
and input files should be straightforward to create and understand.
However, not all OpenPSA formatting is supported, and some additional
assumptions/restrictions are made by SCRAM. See :ref:`opsa_support` for
the format description and current implementation with differences.

In addition to the XML format, the `shorthand_format` is supported indirectly.

.. _`OpenPSA Model Exchange Format v2.0d`:
    http://open-psa.org/joomla1.5/index.php?option=com_content&view=category&id=4&Itemid=19

Steps in XML Input Validation
=============================
#. Several input files can be accepted.
#. An XML input file is validated by RelaxNG_ against the :ref:`schema`.
#. The fault tree validation assumptions/requirements:

    - The hierarchical structure of a fault tree must be strict:

        * The top event is the first gate in the input file fault tree description.
        * Gates must appear as children before they are defined.

    - Event names are not case sensitive.
    - Trailing white spaces are ignored.
    - Names should conform to 'XML NCName datatype' not contain spaces
      and some other special characters.
    - Users are advised not to use special words and characters,
      such as "NOT", "-", ".", ",", "[", "]".
    - Names must be unique if they are public by default.
    - Names must be unique only locally if they are private. [not supported]
    - Primary events and gates with several parents are allowed.
      These events may appear in several places in the fault tree.
    - Proper 'include directive' formatting. [not supported]

#. Additional validation of fault trees and values of parameters is performed:

    - Each gate has a correct number of children.
    - The same child appearing twice or more for one parent is an error.
    - All intermediate events have at least one parent.
    - Values of parameters are correct, i.e., non-negative for probabilities.
    - All events must be defined for probability calculations.

#. Throw an error with a message (a file name, line numbers, types of errors):

    - Report a cyclic tree.
    - Report unsupported gates and events.
    - Report missing event descriptions.
    - Throw an error if an event is being redefined.

#. Warnings for potential errors:

    - Orphan primary events.


.. _schema:

Validation Schemas
==================

- `RelaxNG Schema <https://github.com/rakhimov/scram/blob/master/share/input.rng>`_

.. _shorthand_format:

Shorthand Input Format
======================

A more convenient format than the XML for writing simple fault trees utilizes
a shorter notation for gates ('&', '|', '@', '~', '^') and
basic events to create a collection of Boolean equations.
The shorthand format can be converted into the XML format with `this script`_.

.. _`this script`:
    https://github.com/rakhimov/scram/blob/master/scripts/shorthand_to_xml.py


Input File Examples
===================

Fault Tree Input File
---------------------
.. highlight:: xml
.. literalinclude:: two_train.xml

Shorthand version
-----------------
.. literalinclude:: shorthand_two_train.txt

.. _RelaxNG: http://relaxng.org/
