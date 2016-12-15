##################################
The Open-PSA Model Exchange Format
##################################

Open Probabilistic Safety Assessment Initiative hopes
to bring international community of PSA together
to improve the tools, techniques, and quality of PSA
in a non-competitive and open manner
for the benefit of the community [OPSA]_ [MEF]_.

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

In order to facilitate information exchange and quality assurance,
The Open-PSA community has developed a model exchange format([MEF]_) for PSA
that covers most needs to describe the analysis input for PSA tools.
Moreover, the MEF defines the following requirements
for its development and use:

    - Soundness
    - Completeness
    - Clarity
    - Generality
    - Extensibility

The MEF is designed with the "declarative modeling" criterion
"to present a more informative view of the actual systems, components,
and interactions which the model represents".
This paradigm is followed from the structured programming techniques.

More information about the initiative and format can be found on http://open-psa.org


.. _opsa_support:

Currently Supported Open-PSA MEF Features
=========================================

The difference between `the Open-PSA MEF schema <https://github.com/open-psa/schemas/>`_
and SCRAM's :ref:`schema` can be used
to identify the supported and unsupported features.

- Label
- Attributes
- Public and Private Roles
- Fault Tree Layer

    * Components
    * Basic events
    * House events

        + Boolean constant

    * Gates

        + Nested formulae

- Model Data
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
    * Arithmetic expressions
    * Random deviate (normal, log-normal, histogram, uniform, gamma, beta)
    * Built-in expressions (exponential with two parameters,
      exponential with four parameters, Weibull)


Deviations from the Open-PSA MEF
================================

- Names and references are case-sensitive
  and restricted to fewer characters and combinations.
  :ref:`naming_rules` section contains more information on these restrictions.
- House events must be defined explicitly
  for analysis with probability information.
- The correct number of gate-formula arguments is required.
- Orphan primary events are reported as warning.
- Unused parameters are reported as warning.
- Redefinition of containers, events, and parameters is an error.
- Common cause model levels for factors are required
  and must be strictly sequential in ascending order.
- Attributes are not inherited.
- An empty text in XML attributes or elements is considered an error.


Translators to the Open-PSA MEF
===============================

Various translators from other formats, such as Aralia,
can be found in `the Open-PSA MEF translators repository`_.

.. _the Open-PSA MEF translators repository: https://github.com/open-psa/translators/
