#############################
OpenPSA Model Exchange Format
#############################

Open Probabilistic Safety Assessment Initiative hopes to bring international
community of PSA together to improve the tools, techniques, and quality
of PSA in a non-competitive and open manner for the benefit of the community.

The initiative deals with the following issues in the current PSA:

    - Quality assurance of calculations
    - Unfounded reliance on numerical approximations and truncation
    - Portability of the models between different software
    - Clarity of the models
    - Completeness of the models
    - Modeling of human actions
    - Better visualization of PSA results
    - Difficulty of different software working with the same PSA model
    - Lack of data and software backward and forward compatibility
    - No universal format for industry data

In order to facilitate information exchange and quality assurance, OpenPSA
community has developed a model exchange format(MEF) for PSA that covers
most needs to describe the analysis input for PSA tools. Moreover, the MEF
defines the following requirements for its development and use:

    - Soundness
    - Completeness
    - Clarity
    - Generality
    - Extensibility

The MEF is designed with the "declarative modeling" criterion to
"present a more informative view of the actual systems, components, and
interactions which the model represents". This paradigm is followed from the
structured programming techniques.

More information about the initiative and format can be found on
http://open-psa.org


.. _opsa_mef_schema:

OpenPSA MEF Schemas
===================

- `MEF RelaxNG Schema <https://github.com/rakhimov/scram/blob/master/share/open-psa/mef.rng>`_
- `MEF RelaxNG Compact Schema <https://github.com/rakhimov/scram/blob/master/share/open-psa/mef.rnc>`_
- `MEF DTD <https://github.com/rakhimov/scram/blob/master/share/open-psa/mef.dtd>`_


.. _opsa_support:

Currently Supported OpenPSA MEF Features
========================================

The difference between :ref:`opsa_mef_schema` and :ref:`schema` can be used
to identify the supported and unsupported features.

- Label
- Attributes
- Fault Tree Layer

    * Basic events
    * House events

        + Boolean constant

    * Gates

        + Nested formulae

- Model data
- Common Cause Failure Groups

    * beta-factor
    * MGL
    * alpha-factor
    * phi-factor

- Parameters
- Expressions

    * Constant expressions
    * System mission time
    * Parameter
    * Random deviate (normal, log-normal, histogram, uniform, gamma, beta)
    * Built-in expressions (exponential with two parameters,
      exponential with four parameters, Weibull)


Deviations from OpenPSA MEF
===========================

- Names and references are not case-sensitive and restricted to fewer
  characters and combinations. :ref:`naming_rules` section contains more
  information on these restrictions.
- House events must be defined explicitly for analysis with probability
  information.
- The correct number of a gate's children is required.
- Orphan primary events are reported as warning.
- Unused parameters are reported as warning.
- Redefinition of events is considered an error instead of warning.
- Common cause model levels for factors are required and must be strictly
  sequential in ascending order.
- Attributes are not inherited.


OpenPSA MEF Converters
======================

- `Python script`_ for :ref:`shorthand_format` to OpenPSA MEF XML conversion.

.. _`Python script`:
    https://github.com/rakhimov/scram/blob/master/scripts/shorthand_to_xml.py
