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
- Bdd and Zbdd friendship is a design smell.
  (Access controlled Bdd::Consensus for Zbdd needs Bdd::Function outside of Bdd.)
- FetchTable() functions (Bdd/Zbdd) may not make sense semantically.
- ConvertBddPI is an easy-to-get-wrong name. (There is ConvertBdd.)
- IGate in Boolean graph is confusing (There is Gate in event.h)
- Preprocessing contracts need review, update, and clarification.
- Migrate "Quick Installation" to "Installation" web page.
  (Windows installation is needed to get any value out of this migration.)
- Virtually everything is under one namespace.
- The violation of a basic guarantee (memory leaks)
  due to circular references in incorrect models
  (formulas, parameters, containers).
- Static variables of class type with dynamic initialization.
