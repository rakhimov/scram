##################################
Coding Style and Quality Assurance
##################################

************
Code Quality
************

Coding Styles
=============

This project adheres to the following coding styles:

    #. `Google C++ Style Guide (GCSG)`_
    #. `Google Python Style Guide (GPSG)`_
    #. `PEP 8 -- Style Guide for Python Code (PEP8)`_
    #. `KDE CMake Coding Style`_
    #. `Qt Coding Style`_, `Qt Creator Coding Rules`_ for the GUI Development
    #. `Google Shell Style Guide`_

.. _Google C++ Style Guide (GCSG): https://google.github.io/styleguide/cppguide.html
.. _Google Python Style Guide (GPSG): https://google.github.io/styleguide/pyguide.html
.. _PEP 8 -- Style Guide for Python Code (PEP8): https://www.python.org/dev/peps/pep-0008/
.. _KDE CMake Coding Style: https://community.kde.org/Policies/CMake_Coding_Style
.. _Qt Coding Style: http://wiki.qt.io/Coding-Conventions
.. _Qt Creator Coding Rules: https://doc-snapshots.qt.io/qtcreator-extending/coding-style.html
.. _Google Shell Style Guide: https://google.github.io/styleguide/shell.xml


Deviations from the GCSG
------------------------

- Exceptions are allowed.
- Name mutator functions without ``set_`` prefix.
- Multiple *implementation* inheritance is allowed (mostly for mixins).
- One-liner ``if statements`` are not allowed.
  It interferes with interactive debugging and
  code coverage analysis tools (may hide uncovered lines).


Deviations from the Qt Style
----------------------------

- 80-character line length limit (vs. 100).
- Exceptions are allowed.
- ``using-directives`` are forbidden.
- Prefer anonymous ``namespace`` to ``static`` keyword.
- The GCSG style ``include`` of headers:

    * Sections:

        #. Related header (including ``ui`` header)
        #. C-system headers
        #. C++ system headers
        #. Qt headers
        #. Other libraries' headers
        #. Project Core headers
        #. Project GUI headers

    * Sections are grouped by a blank line.
    * Within each section the includes are ordered alphabetically.

- Class Format:

    * Section order:

        #. ``public:`` member functions
        #. ``signals:`` (public by default in Qt5)
        #. ``public slots:``
        #. ``protected:`` member functions
        #. ``protected slots:``
        #. ``private:`` member functions
        #. ``private slots:``
        #. ``private:`` all data members

    * No blank lines after access specifiers.

    * One blank line before access specifiers except for the first one.

    * Declaration order (the GCSG style):

        #. Using declarations, Typedefs, Structs/Classes, and Enums.
        #. Static const data members
        #. Constructors
        #. Destructors
        #. Methods
        #. Data members

- Automatic connection of signals and slots is forbidden.

- Using literal ``0`` for pointers is forbidden.
  Only ``nullptr`` is allowed for null pointers.


Additional Coding Conventions
-----------------------------

- Use *modern C++* (C++14).
  Refer to `C++ Core Guidelines`_ for best practices.

- Do not use ``inline``
  when defining a function in a class definition.
  It is implicitly ``inline``.
  Do not use ``inline`` as an optimization hint for the compiler.
  Only reasonable use of ``inline`` is for the linker to avoid violating the ODR.

- ``!bool`` vs. ``bool == false``

    1. Prefer using the negation and implicit conversions to ``bool``
       only with Boolean or Boolean-like values
       (``bool``, ``nullptr``, ``0`` for non-existence, etc.).
       An example abuse of the negation and implicit conversion to ``bool`` would be:

    .. code-block:: cpp

        double checked_div(double x, double y) {
            if (!y)                      // Bad. Looks like 'if (no y) then fail'.
                throw domain_error("");  // More explicit (y == 0) is better.
            return x / y;
        }

    2. However, if the expression is long (3 or more parts),
       prefer comparing with the value (``false``, ``0``, etc.) explicitly for readability:

    .. code-block:: cpp

       if (var.getter().data().empty() == false);

    3. Avoid inverted or negated logic if possible.

.. _C++ Core Guidelines: https://github.com/isocpp/CppCoreGuidelines


Core C++ Code
~~~~~~~~~~~~~

- Exceptions are forbidden in **analysis code**.

- RTTI (typeid, dynamic_cast, dynamic_pointer_cast, etc.)
  is forbidden in **analysis code**.

- `Defensive Programming`_.
  Check all preconditions, postconditions, invariants, and assumptions
  with the ``assert`` macro wherever possible in **analysis code**.
  Consider supplying an error message to clarify the assertion,
  for example, ``assert(!node->mark() && "Detected a cycle!")``.

- If function input parameters or return values
  are pointers (raw or smart),
  they are never null pointers
  unless explicitly specified.
  Null pointer based logic must be
  rare, localized, and explicit.

- Consider supplying a typedef or alias declaration
  for common smart pointers.

    * ``ClassNamePtr`` for shared, unique, and intrusive pointers
    * ``ClassNameWeakPtr`` for weak pointers

- Function call qualification conventions:

    * Unqualified calls customizable by or relying on the ADL
      must make it explicit in the documentation and comments.

    * In definitions of member functions:

        - Explicitly qualify calls to inherited non-virtual member functions
          with the corresponding base class names, e.g., ``BaseClassName::Foo()``.
        - Qualify virtual functions to be overridden by design as ``this->Foo()``.
        - Qualify a call to a free function with its namespace, e.g., ``scram::Foo()``.

    * In definitions of free functions,
      calls to other free functions in the enclosing namespace can be unqualified.

- Declare a getter function before a setter function
  for a corresponding member variable.

- Declare getter and setter functions before other complex member functions.

- Domain-specific ``Probability`` naming rules:

    * If a probability variable is a member variable of a class
      abbreviate it to ``p_``.
      Its getter/setter functions should have
      corresponding names, i.e., ``p()`` and ``p(double value)``.
      Append extra description after ``p_``, e.g., ``p_total_`` (a la Semantic Hungarian).
      Avoid abbreviating the name to ``prob``
      or fully spelling it to ``probability``.

    * For non-member probability variables:

        + Prefer prefixing with ``p_`` (a la Semantic Hungarian)
          if the name has more description to the probability value, e.g., ``p_not_event``.
        + Prefer ``prob`` abbreviation
          for single word names indicating general probability values.

    * Prefer spelling ``Probability`` fully for cases not covered above
      (class/function/namespace/typedef/...), e.g., ``CalculateProbability``.
      Avoid abbreviating the name, e.g., ``CalculateProb``.

- Prefer the terminology and concepts of Boolean algebra and graph theory
  to the terminology and concepts of risk analysis in **analysis code**.
  For example, a Boolean product is more general and appropriate for analysis facilities
  than cut sets or prime implicants.

    * There is no Boolean operator for the K-out-of-N logic.
      This gate in fault tree analysis has many names
      (Voting, Combination, atleast, K/N),
      and there doesn't seem to be a consensus among sources and tools.
      The Open-PSA MEF "atleast" best captures the nature of the gate;
      however, the "atleast" is awkward to use in code and API
      (Atleast vs. AtLeast vs. atleast vs. at_least).
      In SCRAM, the "vote" word must be used consistently
      to represent this gate in code and API.
      The code that deals with the Open-PSA MEF may use the "atleast".

- In performance-critical **analysis code**
  (BDD variable ordering, Boolean formula rewriting/preprocessing, etc.),
  avoid platform/implementation-dependent constructs
  (iterating over unordered containers, unstable sorts,
  using an object address as its identity, etc.).
  The performance profile must be stable across platforms.

.. _Defensive Programming: https://www.youtube.com/watch?v=1QhtXRMp3Hg


GUI Code
~~~~~~~~

- Avoid Qt containers whenever possible.
  Prefer STL/Boost containers and constructs.

- Upon using Qt containers and constructs,
  stick to their STL API and usage style
  as much as possible.
  Avoid the Java-style API as much as possible.

- Avoid overloading signals and slots.

- Avoid default arguments in signals and slots.

- Prefer Qt5 style connections without ``SIGNAL``/``SLOT`` macros.

- Prefer normalized signatures in connect statements with ``SIGNAL``/``SLOT`` macros.

- Prefer Qt Designer UI forms over hand-coded GUI.

    * The UI class member must be aggregated as a private pointer member
      and named ``ui`` without ``m_`` prefix (default in Qt Creator).
    * Don't repeat ``includes`` provided by the ``ui`` header.

- Common Qt includes may be omitted,
  for example, ``QString``, ``QList``, ``QStringList``, and ``QDir``.

- Avoid using forward declaration of Qt library classes.
  Just include the needed headers.


Monitoring Code Quality
=======================

C++
---

#. Performance profiling with Gprof, Valgrind_, and ``perf``
#. Code coverage check with Gcov_ and reporting with Coveralls_
#. Memory management bugs and leaks with Valgrind_
#. Static code analysis with Coverity_ and CppCheck_
#. Cyclomatic complexity analysis with Lizard_
#. Google style conformance check with Cpplint_
#. Common C++ code problem check with cppclean_
#. Consistent code formatting with ClangFormat_
#. Component dependency analysis with cppdep_

.. _Gcov: https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
.. _Coveralls: https://coveralls.io/github/rakhimov/scram
.. _Valgrind: http://valgrind.org/
.. _Coverity: https://scan.coverity.com/projects/2555
.. _CppCheck: https://github.com/danmar/cppcheck/
.. _Lizard: https://github.com/terryyin/lizard
.. _Cpplint: https://github.com/cpplint/cpplint
.. _cppclean: https://github.com/myint/cppclean
.. _ClangFormat: http://clang.llvm.org/docs/ClangFormat.html
.. _cppdep: https://pypi.python.org/pypi/cppdep


Python
------

#. Code quality and style check with Pylint_
#. Profiling with PyVmMonitor_
#. Code coverage check with coverage_ and reporting with Codecov_
#. Continuous code quality control on Landscape_ with Prospector_

.. _Pylint: https://www.pylint.org/
.. _PyVmMonitor: http://www.pyvmmonitor.com/
.. _coverage: https://coverage.readthedocs.io/en/latest/
.. _Codecov: https://codecov.io/github/rakhimov/scram
.. _Landscape: https://landscape.io/
.. _Prospector: https://github.com/landscapeio/prospector


Targets
-------

====================   ==================   ==================
Metric                 Before Release       On Release
====================   ==================   ==================
C++ Code Coverage      80%                  95%
C++ Defect Density     0.5 per 1000 SLOC    0.35 per 1000 SLOC
CCN                    15                   15
Python Code Coverage   80%                  95%
Pylint Score           9.0                  9.5
Documentation          Full                 Full
====================   ==================   ==================

.. note:: C++ defects that count towards the defect density include
          analysis errors, Coverity report, memory leaks,
          and *known* critical bugs.

.. note:: Utility scripts written in Python are exempt from the test coverage requirement.


Testing and Continuous Integration
==================================

In order to facilitate better software quality and quality assurance,
full test coverage is attempted
through unit, integration, regression, and benchmarking tests.
The following tools are used for this purpose:

    - GoogleTest_
    - Nose_

These tests are automated,
and continuous integration is provided by `Travis CI`_ and AppVeyor_.

Guided fuzz testing is performed
with auto-generated analysis input files
to discover bugs, bottlenecks, and assumption failures.

.. _GoogleTest: https://github.com/google/googletest
.. _Nose: httpss://nose.readthedocs.io/en/latest/
.. _Travis CI: https://travis-ci.org/rakhimov/scram
.. _AppVeyor: https://ci.appveyor.com/project/rakhimov/scram


References for testing and quality assurance
--------------------------------------------

- `Software Testing Fundamentals`_
- `Software Testing Tutorial`_
- `ISO Standards for Software Testing`_
- `Introduction to Test Driven Development`_

.. _Software Testing Fundamentals: http://softwaretestingfundamentals.com/
.. _Software Testing Tutorial: http://www.tutorialspoint.com/software_testing/
.. _ISO Standards for Software Testing: http://softwaretestingstandard.org/
.. _Introduction to Test Driven Development: http://agiledata.org/essays/tdd.html


Version control and Versioning
==============================

- `Git SCM`_
- `Branching Model`_
- `Writing Good Commit Messages`_
- `On Commit Messages`_
- `Atomic Commit`_
- `Semantic Versioning`_

.. _Git SCM: https://git-scm.com/
.. _Branching Model: http://nvie.com/posts/a-successful-git-branching-model/
.. _Writing Good Commit Messages: https://github.com/erlang/otp/wiki/Writing-good-commit-messages
.. _On Commit Messages: http://who-t.blogspot.com/2009/12/on-commit-messages.html
.. _Atomic Commit: https://en.wikipedia.org/wiki/Atomic_commit#Atomic_commit_convention
.. _Semantic Versioning: http://semver.org/


*************
Documentation
*************

.. image:: http://www.osnews.com/images/comics/wtfm.jpg
    :align: center

Good documentation of the code and functionality is
the requirement for maintainability and evolution of the project.

The project adheres to the Documentation Driven Development model (`DDD talk by Corey Oordt`_),
following the best practices of `Agile Documentation`_,
Google Documentation Guide Philosophy_ and `Best Practices`_.

The documentation for the project is maintained in the reStructuredText_ format,
and the final representations are dynamically generated with Sphinx_
in various formats (html, pdf, LaTeX).

The code documentation is dynamically generated with Doxygen_,
which also verifies full documentation coverage.

The source text of the documentation in the code and the reST format
must be formatted consistently and with `Semantic Linefeeds`_
for maintainability and version control.

.. _Doxygen: http://doxygen.org/
.. _Sphinx: http://sphinx-doc.org/
.. _reStructuredText: http://docutils.sourceforge.net/rst.html
.. _DDD talk by Corey Oordt: http://pyvideo.org/video/441/pycon-2011--documentation-driven-development
.. _Agile Documentation: http://www.agilemodeling.com/essays/agileDocumentationBestPractices.htm
.. _Philosophy: https://github.com/google/styleguide/blob/gh-pages/docguide/philosophy.md
.. _Best Practices: https://github.com/google/styleguide/blob/gh-pages/docguide/best_practices.md
.. _Semantic Linefeeds: http://rhodesmill.org/brandon/2012/one-sentence-per-line/


Conventions in Documentation "Source Text"
==========================================

General
-------

- Prefer the :ref:`Aralia_format` for the Boolean formula documentation.
  This format uses the C-style bit-wise logical operators for equations.


reST Documentation Style
------------------------

- Semantic Linefeeds
- Two blank lines between sections with bodies
- One blank line after a header before its body
- Part ``#`` overlined and underlined
- Chapter ``*`` overlined and underlined
- Section underlining and order ``=``, ``-``, ``~``, ``^``, ``+``
- Point nesting and order ``-``, ``*``, ``+``
- 4-space indentation
- 100 character line limit
  (except for links and paths)
- No trailing whitespace characters
- No tabs (spaces only)
- No excessive blank lines at the end of files


Core Code Documentation Style
-----------------------------

- Semantic Linefeeds
- Doxygen comments with ``///`` and ``///<``
- Comment ordering:

    #. description
    #. tparam
    #. param
    #. returns
    #. pre
    #. post
    #. throws
    #. note
    #. warning
    #. todo

- Leave one Doxygen blank line between sections
- Always specify input and output parameters with
  ``@param[in,out] arg  Description...``

    * Two spaces between parameter and its description
    * The same formatting for template parameters ``@tparam T  Type desc...``

- The two-space formatting for ``@throws Error  Description``
- In-code TODOs with Doxygen ``/// @todo``
  so that Doxygen picks them up.


GUI Code Documentation Style
----------------------------

- Semantic Linefeeds
- Leverage Qt Creator for auto-documentation with Doxygen
  (Javadoc style and ``///<`` for one-liners)
- The same organization of Doxygen sections as in the core code.


********************
XML Formatting Style
********************

- 2-space indentation
- No tabs (spaces only)
- No trailing whitespace characters
- No excessive blank lines
- No spaces around tag opening and closing brackets: ``<``, ``/>``, ``<\``, ``>``.
- Only one space between attributes
- No spaces around ``=`` in attribute value assignment
- Prefer 100 character line limit
- Avoid putting several elements on the same line
- UTF-8 encoding
