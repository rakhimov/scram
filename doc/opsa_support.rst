.. _opsa_support:

########################################
Currently Supported OpenPSA MEF Features
########################################

- Fault Tree Description:

  * Non-nested gates (formula).

- Basic Event Description:

  * Float probability

- House Event Description:

  * Boolean probability description

- Model data


*****************************************
Deviations from OpenPSA MEF
*****************************************

- Names are not case-sensitive
- House events do not default to false state implicitly. They must be defined.
- First event for the fault tree must be a top event gate.
- Fault tree input structure must be top-down. [not checked currently]
- The correct number of a gate's children is required.
