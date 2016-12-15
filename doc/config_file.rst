###################
Configuration Files
###################

In order to analyze complex analysis models
with several input files and tailored configurations,
it is more convenient to use a configuration file than command-line arguments.
Additional command-line options can be provided
to override or suppress the options from a configuration file.

The configuration file is an XML file
that must conform to the specific schema provided bellow.


Validation Schemas
==================

- `RELAX NG Schema <https://github.com/rakhimov/scram/blob/master/share/config.rng>`_


Configuration File Example
==========================

.. literalinclude:: example/config.xml
    :language: xml
