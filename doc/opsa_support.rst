#############################
OpenPSA Model Exchange Format
#############################

Open Probabilistic Safety Assessment Initiative hopes to bring international
community of PSA together to improve the tools, techniques, and quality
of PSA in an non-competitive and open manner for the benefit of the
community.

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

.. _opsa_support:

****************************************
Currently Supported OpenPSA MEF Features
****************************************

- Label for elements

- Attributes list

- Fault Tree Description:

  * Non-nested gates (formula)

- Basic Event Description

- Expressions:

    * Constant expressions
    * System mission time
    * Parameter
    * Random deviate (normal, log-normal, histogram, uniform, gamma, beta)
    * Built-in expressions (exponential with two parameters, exponential with four parameters, Weibull)

- House Event Description:

  * Boolean probability description

- Model data


***************************
Deviations from OpenPSA MEF
***************************

- Names are not case-sensitive.
- House events do not default to false state implicitly. They must be defined.
- Expressions are not optional for Basic events.
- First gate for the fault tree must be a top event gate.
- Fault tree input structure must be top-down. Parents must appear before
  children.
- The correct number of a gate's children is required.
- Unused primary events are ignored but reported as warning.
- Double definition of events is considered an error instead of warning.
