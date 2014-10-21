########################
SCRAM Bugs and Problems
########################

`Issues on GitHub <https://github.com/rakhimov/scram/issues>`_

Currently Found Bugs
====================

Minor Issues
====================

- Input files with currently unsupported advanced OpenPSA MEF features may
  cause program failures without any warnings.

- The current algorithms are not optimized for complex analysis, but
  more optimization is planned after implementing main features.

- Importances are calculated assuming non-coherent trees. If assumptions fail,
  no warning is given. The current importance analysis is very limited.
