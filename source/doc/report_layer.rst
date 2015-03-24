############
Report Layer
############

.. note::
    This feature is under development.

The results of calculations are reported in the XML format suggested by
`OpenPSA Model Exchange Format v2.0d`_. It is expected that the standardized
report format be used by other tools to further analyze and operate on
the results of PSA tools. The report contains two constructs:

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

.. _`OpenPSA Model Exchange Format v2.0d`:
    http://open-psa.org/joomla1.5/index.php?option=com_content&view=category&id=4&Itemid=19

Validation Schemas
==================

- `RelaxNG Schema <https://github.com/rakhimov/scram/blob/master/share/report_layer.rng>`_
