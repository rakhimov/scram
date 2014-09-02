##################
SCRAM Input Files
##################

SCRAM uses XML input files to describe analysis. See :ref:`xml_tools` for
more convenient writing of XML input files.

Currently, only fault trees are accepted for analysis. The input file format
follows `OpenPSA Model Exchange Format v2.0d`_ or later.
However, not all OpenPSA formatting is supported, and some additional
assumptions/restrictions are made by SCRAM. See :ref:`opsa_support` for the current
implementation and differences.

.. _`OpenPSA Model Exchange Format v2.0d`:
    http://open-psa.org/joomla1.5/index.php?option=com_content&view=category&id=4&Itemid=19

Steps in XML Input Validation
=============================
#. Only one input file is accepted.
#. The XML input file is validated by RelaxNG_ against the :ref:`schema`.
#. The fault tree validation assumptions/requirements:

    - The hierarchical structure of a fault tree must be strict:

        + The top event is the first gate in the input file fault tree description.
        + The parent must be defined before its children.

    - Event names are not case sensitive.
    - Trailing white spaces are ignored.
    - Names should conform to 'XML NCName datatype' not contain spaces
      and some other special characters.
    - Names must be unique if they are public by default.
    - Names must be unique only locally if they are private. [not supported]
    - Primary and intermediate events with several parents are allowed.
      These events may appear in several places in the tree.
    - Proper 'include directive' formatting. [not supported]

#. Additional validation of fault trees and values of parameters is performed:

    - Each gate has a correct number of children.
    - All intermediate events have at least one parent.
    - Values of parameters are correct, i.e., non-negative for probabilities.
    - All events must be defined for probability calculations.

#. Throw an error with a message (a file name, line numbers, types of errors):

    - Report a cyclic tree.
    - Report unsupported gates and events.
    - Report missing event descriptions.
    - Throw an error if an event is defined doubly.
    - Ignore primary events that are not initialized in the tree when assigning
      model data. (Only warning [not supported])


.. _schema:

Validation Schemas
==================

- `RelaxNG Schema <https://github.com/rakhimov/SCRAM/blob/master/share/scram.rng>`_
- `RelaxNG Compact <https://github.com/rakhimov/SCRAM/blob/master/share/scram.rnc>`_
- `DTD Schema <https://github.com/rakhimov/SCRAM/blob/master/doc/open-psa/mef.dtd>`_

Input File Examples
===================

Fault Tree Input File
---------------------
.. highlight:: xml
.. literalinclude:: two_train.xml


.. _RelaxNG: http://relaxng.org/
