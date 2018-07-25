#############
Project Files
#############

.. note:: This is not a MEF standard, so portability/stability is not guaranteed.

With complex models
consisting of several input files and requiring tailored configurations,
it may be more convenient to use a project file than command-line arguments.
Additional command-line arguments can be provided
and may be used to override some options in a project file.

The project file format is an XML file
that must conform to the specific schema linked bellow.


Validation Schemas
==================

- `RELAX NG Schema <https://github.com/rakhimov/scram/blob/master/share/project.rng>`_


Project File Example
====================

.. literalinclude:: example/project.xml
    :language: xml
