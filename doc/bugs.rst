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
- Abuse of smart pointers (shared pointers).
  Pollution of function interfaces.
- mef::Gate and mef::Formula have 'type' field instead of 'operator' (reserved in C++)
- Bdd and Zbdd friendship is a design smell.
  (Access controlled Bdd::Consensus for Zbdd needs Bdd::Function outside of Bdd.)
- The violation of the basic guarantee (memory leaks)
  due to circular references in incorrect models
  (formulas, parameters, containers).
- Static variables of class type with dynamic initialization.
- Performance profile across platforms is not stable.
- Inefficient double-lookup anti-pattern for containers.
  For example, ``if (map.count(key)) throw error(); map.emplace(key, value);``.
