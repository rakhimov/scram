###############
Bugs and Issues
###############

- `Issues on GitHub <https://github.com/rakhimov/scram/issues>`_


Technical Issues
----------------

.. note:: The following list contains
          non-critical or questionable
          low-level technical issues.

- Static variables of class type with dynamic initialization.
- The performance profile across platforms is not stable.
- The builds are very demanding for the project size:
  500 MiB/job memory and 8 min total time.
  Major contributing factors may be
  the template-heavy code
  and coupled physical layout of the project components.
- Test coverage instruments fail to tally some inline functions and function templates
  (gcc/gcov no-inline limitations).
