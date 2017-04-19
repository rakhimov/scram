############
Report Files
############

****************
The Open-PSA MEF
****************

The results of calculations are reported in the XML format
suggested by the Open-PSA Model Exchange Format ([MEF]_).
SCRAM-specific extra report XML elements and attributes are added
on top of the MEF report format in a compatible way.
It is expected that the standardized report format be used by other tools
to further analyze and operate on the results of PSA tools.


Encodings
=========

There is no specification for file encodings in the Open-PSA MEF.
SCRAM results are reported with the UTF-8 character set
regardless of what the input file encodings are.


Time
====

Reporting of analysis time and duration is not specified in the MEF standard.
For informational purposes, SCRAM reports analysis duration in seconds,
and the UTC date-time is formatted in ISO 8601 extended form.


Validation Schemas
==================

- `RELAX NG Schema <https://github.com/rakhimov/scram/blob/develop/share/report.rng>`_


***************
Post-processing
***************

It is expected that the results of analysis
are processed by other tools than the SCRAM core.
The GUI front-end, Python or R scripts, and XML query tools are better suited
to filter, group, sort, and do other data manipulations and visualization.

Suggested tools:

    - SCRAM GUI front-end
    - R with `FaultTree.SCRAM <https://rdrr.io/rforge/FaultTree.SCRAM/>`_
    - Python with `lxml <http://lxml.de/>`_
    - `BaseX <http://basex.org>`_


Report File Example
===================

.. literalinclude:: example/report.xml
    :language: xml
