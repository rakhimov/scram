###########
Input Files
###########

SCRAM uses XML input files to describe analysis. See :ref:`xml_tools` for
more convenient writing and reading of XML input files.

Currently, only fault trees are accepted for analysis, and one model per run is
assumed. The input file format follows `OpenPSA Model Exchange Format v2.0d`_
or later. The extensive description is given in the above format documentation
by OpenPSA, and input files should be straightforward to create and understand.
However, not all OpenPSA formatting is supported, and some additional
assumptions/restrictions are made by SCRAM. See :ref:`opsa_support` for the
format description and current implementation with differences.

In addition to the XML format, the :ref:`shorthand_format` is supported
indirectly.

.. _`OpenPSA Model Exchange Format v2.0d`:
    http://open-psa.org/joomla1.5/index.php?option=com_content&view=category&id=4&Itemid=19


Steps in XML Input Validation
=============================

#. Several input files are accepted that describe one model.

    - The first file must define the name, label, and attributes of the model.
      Other files with this kind of information are ignored without a warning.
      This feature allows reuse of files from other models without the need for
      include directives.

#. An XML input file is validated by RelaxNG_ against the :ref:`schema`.
#. The fault tree validation assumptions/requirements:

    - Event names and references are not case-sensitive.
    - Leading and trailing whitespace characters are trimmed.
    - Names and references should conform to the rules defined in
      :ref:`naming_rules` section.
    - Public names must be unique globally, and private names must be unique
      locally within containers.
    - References are followed according to the public and private roles
      described in the OpenPSA MEF section
      "IV.3.2. Solving Name Conflicts: Public versus Private Elements".

#. Additional validation of fault trees and values of parameters is performed:

    - Each gate has the correct number of children.
    - The same child appearing two or more time for one parent is an error.
    - Values of expressions and parameters are correct, i.e., non-negative for
      probabilities.
    - All events must be explicitly defined for probability calculations.

#. Error messages (a file name, line numbers, types of errors):

    - Report a cyclic tree.
    - Report a cyclic parameter with expressions.
    - Report missing element descriptions.
    - Throw an error if an event is being redefined.

#. Warnings for potential errors:

    - Orphan primary events.
    - Unused parameters.


.. _naming_rules:

Identifiers and References
--------------------------

Names are used as identifiers for many constructs, such as events and
parameters. To avoid ambiguity and achieve consistency, names must conform to
the intersection of the following rules.

- Consistent with `XML NCName datatype`_

    * The first character must be alphabetic.
    * May contain alphanumeric characters and special characters like "_", "-".
    * No whitespace or other special characters like ":", ",", "/", etc.

- No double dashes ("--")
- No trailing dash
- No periods (".")

References to constructs, such as gates, events, and parameters, may include
names of fault trees or components to access public or private members. This
feature requires a period (".") between names; thus references may follow the
pattern ("fault_tree.component.event").

.. _`XML NCName datatype`:
    http://stackoverflow.com/questions/1631396/what-is-an-xsncname-type-and-when-should-it-be-used


.. _schema:

Validation Schemas
==================

- `RelaxNG Schema <https://github.com/rakhimov/scram/blob/master/share/input.rng>`_


.. _shorthand_format:

Shorthand Input Format
======================

A more convenient format than the XML for writing simple fault trees utilizes
a shorter notation for gates ('&', '|', '@', '~', '^') and events to create a
collection of Boolean equations. The shorthand format can be converted into the
XML format with `this script`_.

.. _`this script`:
    https://github.com/rakhimov/scram/blob/master/scripts/shorthand_to_xml.py


Input File Examples
===================

Fault Tree Input File
---------------------

.. highlight:: xml
.. literalinclude:: two_train.xml


Shorthand Version
-----------------

.. literalinclude:: shorthand_two_train.txt

.. _RelaxNG: http://relaxng.org/
