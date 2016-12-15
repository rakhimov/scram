############
Report Layer
############

****************
The Open-PSA MEF
****************

The results of calculations are reported in the XML format
suggested by the Open-PSA Model Exchange Format ([MEF]_).
It is expected that the standardized report format be used by other tools
to further analyze and operate on the results of PSA tools.
The report contains two constructs:

    - Information about the software, algorithms, and options.

        * Software

            + Version
            + Contact organization (editor, vendor...)

        * Calculated quantities

            + Name
            + Mathematic definitions
            + Approximations used

        * Calculations methods

            + Name
            + Limits (e.g. number of basic events, sequences, cut sets)
            + Preprocessing techniques (modularization, rewritings, ...)
            + Handling of success terms
            + Cutoffs, if any (absolute, relative, dynamic, ...)
            + Are recovery rules or exchange events applied?
            + Extra-logical methods used
            + Secondary software necessary
            + Warning and caveats
            + Calculation time

        * Features of the model

            + Name
            + Number of gates, basic events, house events, fault trees, event
              trees, functional events, initiating events

        * Feedback

            + Success or failure reports

    - Results

        * Minimal cut sets (and prime implicants)
        * Statistical measures (with moments)
        * Curves
        * Probability/frequency, importance factors, sensitivity analyses, ...


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

- `RELAX NG Schema <https://github.com/rakhimov/scram/blob/master/share/report_layer.rng>`_


***************
Post-processing
***************

It is expected that the results of analysis
are processed by other tools than the SCRAM core.
The GUI front-end, Python scripts, and XML query tools are better suited
to filter, group, sort, and do other data manipulations and visualization.

Suggested tools:

    - SCRAM GUI front-end
    - `BaseX <http://basex.org>`_
    - Python with `lxml <http://lxml.de/>`_


Report File Example
===================

.. literalinclude:: example/report.xml
    :language: xml
