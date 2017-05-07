###########
Input Files
###########

SCRAM uses XML input files to describe analysis.
One model per run is assumed.
See :ref:`xml_tools` for more convenient writing and reading of XML input files.

The input file format follows
the Open-PSA Model Exchange Format ([MEF]_) version 2.0d or later.
The extensive description is given in the above format documentation by Open-PSA,
and input files should be straightforward to create and understand.
However, not all Open-PSA formatting is supported,
and some additional assumptions/restrictions are made by SCRAM.
See :ref:`opsa_support` for the format description
and current implementation with differences.

In addition to the XML format,
the :ref:`Aralia_format` is supported indirectly.


Encodings
=========

- UTF-8
- UTF-16
- other encodings supported by `GNOME XML Parser`_ on the given platform.

.. _GNOME XML Parser: http://xmlsoft.org/encoding.html


Steps in XML Input Validation
=============================

#. Several input files to describe one model.

    - The first file must define the name, label, and attributes of the model.
      This information about models in the other input files is ignored without a warning,
      which allows reuse of files with analysis constructs from other models.

#. XML input file validation against the `RELAX NG`_ :ref:`schema`.
#. The validation assumptions/requirements:

    - Construct names and references are case-sensitive.
    - Leading and trailing whitespace characters are trimmed.
    - Names and references should conform to the MEF recommendations.
    - Public names must be unique globally,
      and private names must be unique locally within containers.
    - References are followed according to the public and private roles
      described in the Open-PSA MEF section
      *IV.3.2. Solving Name Conflicts: Public versus Private Elements*.

#. Additional validation of fault trees and values of parameters:

    - Each gate or formula must have the correct number of arguments.
    - A duplicate argument in Boolean formulas or gates is an error.
    - Values of expressions and parameters must be valid, i.e., non-negative for probabilities.
    - All events must be explicitly defined for probability calculations.

#. Error messages (with a file name, line numbers, types of errors):

    - Report a cyclic tree (gates or event tree branches).
    - Report a cyclic parameter with expressions.
    - Report a cyclic rule with instructions.
    - Report missing element descriptions.
    - Report an error if an event or a construct is being redefined.

#. Warnings for potential errors:

    - Unused model constructs:

        * Primary events
        * Parameters
        * Event trees
        * Initiating events or event groups
        * Functional events, branches, sequences, rules


.. _schema:

Validation Schemas
==================

- `RELAX NG Schema <https://github.com/rakhimov/scram/blob/master/share/input.rng>`_


.. _Aralia_format:

Aralia Input Format
===================

A more convenient format than the XML for writing simple fault trees
utilizes a shorter notation for gates, operators (``&``, ``|``, ``@``, ``~``, ``^``), and events
to create a collection of Boolean formulas.
The Aralia format can be converted into the XML format with `this script`_.

.. _this script:
    https://github.com/open-psa/translators/blob/master/aralia.py


Input File Examples
===================

Fault Tree Input File
---------------------

.. literalinclude:: example/input.xml
    :language: xml


Aralia Version
--------------

.. literalinclude:: example/aralia_input.txt

.. _RELAX NG: http://relaxng.org/
