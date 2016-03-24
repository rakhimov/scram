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
- Abuse of smart pointers (shared pointers)
- IGate and Formula have 'type' field instead of 'operator' (reserved in C++)
- IGate in Boolean graph is confusing (There is Gate in event.h)
- Bdd and Zbdd friendship is a design smell.
  (Access controlled Bdd::Consensus for Zbdd needs Bdd::Function outside of Bdd.)
- Virtually everything is under one namespace.
- The violation of a basic guarantee (memory leaks)
  due to circular references in incorrect models
  (formulas, parameters, containers).
- Static variables of class type with dynamic initialization.
