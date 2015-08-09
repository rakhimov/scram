###################
Configuration Files
###################

In order to work with complex analysis
with several input files and tailored configurations,
it is more convenient to use a configuration file than command-line arguments.
Moreover, additional command-line options can be provided
to override or suppress the options from a configuration file.

The configuration file is an XML file
that must conform to the specific schema provided bellow.


Validation Schemas
==================

- `RelaxNG Schema <https://github.com/rakhimov/scram/blob/master/share/config.rng>`_


Configuration File Example
==========================

.. highlight:: xml
.. literalinclude:: config.xml
