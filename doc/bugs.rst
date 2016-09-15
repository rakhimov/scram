###############
Bugs and Issues
###############

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_


Technical Issues
----------------

.. note:: The following list contains
          non-critical or questionable
          low-level technical issues.

- Copying Settings around is expensive (~100B)
- mef::Gate and mef::Formula have ``type`` field instead of ``operator`` (reserved in C++)
- Static variables of class type with dynamic initialization.
- Performance profile across platforms is not stable.
