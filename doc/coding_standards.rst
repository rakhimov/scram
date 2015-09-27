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
    #. `Qt Coding Style`_ for the GUI Development
    #. `Google Shell Style Guide`_

.. _Google C++ Style Guide (GCSG): http://google-styleguide.googlecode.com/svn/trunk/cppguide.html
.. _Google Python Style Guide (GPSG): http://google-styleguide.googlecode.com/svn/trunk/pyguide.html
.. _PEP 8 -- Style Guide for Python Code (PEP8): https://www.python.org/dev/peps/pep-0008/
.. _KDE CMake Coding Style: https://techbase.kde.org/Policies/CMake_Coding_Style
.. _Qt Coding Style: http://qt-project.org/wiki/Qt_Coding_Style
.. _Google Shell Style Guide: https://google-styleguide.googlecode.com/svn/trunk/shell.xml


Deviations from the GCSG
------------------------

- Exceptions are allowed
- Prefer streams to ``printf-like`` routines
- Name mutator functions without ``set_`` prefix
- Multiple inheritance is allowed (mostly for mixins)


Additional Coding Conventions
-----------------------------

Core C++ Code
~~~~~~~~~~~~~

- Exceptions are forbidden in **analysis code**.

- Check all preconditions and assumptions
  with the ``assert`` macro wherever possible in **analysis code**.
  Consider supplying an error message to clarify the assertion,
  for example, ``assert(!node->mark() && "Detected a cycle!")``.

- If function input parameters or return values
  are pointers (raw or smart),
  they are never null pointers
  unless explicitly specified.
  Null pointer based logic must be
  rare, localized, and explicit.

- Consider creating a typedef (using declaration/alias)
  for common smart pointers.

    * ``ClassNamePtr`` for shared and unique pointers
    * ``ClassNameWeakPtr`` for weak pointers

- Prefer "modern C++" (C++11).
  Refer to `C++ Core Guidelines`_ for best practices.

- In definitions of class member functions,
  prefix all calls of non-virtual member and inherited functions
  with corresponding class names, i.e., ``ClassName::Foo()``.
  Use ``this->Foo()`` only for virtual functions to be overridden by design.
  Use ``Foo()`` only for free functions in current namespace.

- Declare a getter function before a setter function
  for a corresponding member variable.

- Declare getter and setter functions before other complex member functions.

- Do not use ``inline``
  when defining a function in a class definition.
  It is implicitly ``inline``.

.. _C++ Core Guidelines: https://github.com/isocpp/CppCoreGuidelines


Monitoring Code Quality
=======================

C++
---

#. Performance profiling with Gprof_
#. Code coverage check with Gcov_ and reporting with Coveralls_
#. Test status dashboard on CDash_
#. Memory management bugs and leaks with Valgrind_
#. Static code analysis with Coverity_
#. Cyclomatic complexity analysis with Lizard_
#. Google style conformance check with Cpplint_
#. Common C++ code problem check with cppclean_
#. Consistent code formatting with ClangFormat_

.. _Gprof: https://www.cs.utah.edu/dept/old/texinfo/as/gprof.html
.. _Gcov: https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
.. _Coveralls: https://coveralls.io/r/rakhimov/scram
.. _CDash: http://my.cdash.org/index.php?project=SCRAM
.. _Valgrind: http://valgrind.org/
.. _Coverity: https://scan.coverity.com/projects/2555
.. _Lizard: https://github.com/terryyin/lizard
.. _Cpplint: https://google-styleguide.googlecode.com/svn/trunk/cpplint/
.. _cppclean: https://github.com/myint/cppclean
.. _ClangFormat: http://clang.llvm.org/docs/ClangFormat.html

Python
------

#. Code quality and style check with Pylint_
#. Profiling with PyVmMonitor_
#. Code coverage check with coverage_

.. _Pylint: http://www.pylint.org/
.. _PyVmMonitor: http://www.pyvmmonitor.com/
.. _coverage: http://nedbatchelder.com/code/coverage/


Targets
-------

====================   ==================   ==================
Metric                 Before Release       On Release
====================   ==================   ==================
C++ Code Coverage      80%                  95%
C++ Defect Density     0.5 per 1000 SLOC    0.35 per 1000 SLOC
CCN                    15                   15
Python Code Coverage   80%                  90%
Pylint Score           9.0                  9.5
Documentation          Full                 Full
====================   ==================   ==================


Testing and Continuous Integration
==================================

In order to facilitate better software quality and quality assurance,
full test coverage is attempted
through unit, integration, regression, and benchmarking tests.
The following tools are used for this purpose:

    - GoogleTest_
    - Nose_

These tests are automated,
and continuous integration is provided by `Travis CI`_.

.. _GoogleTest: https://code.google.com/p/googletest/
.. _Nose: https://nose.readthedocs.org/en/latest/
.. _Travis CI: https://travis-ci.org/rakhimov/scram


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
- `Semantic Versioning`_

.. _Git SCM: http://git-scm.com/
.. _Branching Model: http://nvie.com/posts/a-successful-git-branching-model/
.. _Writing Good Commit Messages: https://github.com/erlang/otp/wiki/Writing-good-commit-messages
.. _On Commit Messages: http://who-t.blogspot.com/2009/12/on-commit-messages.html
.. _Semantic Versioning: http://semver.org/


*************
Documentation
*************

Good documentation of the code and functionality is
the requirement for maintainability and evolution of the project
and its acceptance by users.

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
.. _Philosophy: https://github.com/google/styleguide/blob/gh-pages/docguide/best_practices.md
.. _Best Practices: https://github.com/google/styleguide/blob/gh-pages/docguide/best_practices.md
.. _Semantic Linefeeds: http://rhodesmill.org/brandon/2012/one-sentence-per-line/


Conventions in Documentation "Source Text"
==========================================

General
-------

- Prefer :ref:`shorthand_format` for the Boolean formula documentation.
  This format uses the C-style bitwise logical operators for equations.


reST Documentation Style
------------------------

- Semantic Linefeeds
- Two blank lines between sections with bodies
- One blank line after a header before its body
- Title '#' overlined and underlined
- Chapter '*' overlined and underlined
- Section underlining and order '=', '-', '~', '^', '+'
- Point nesting and order '-', '*', '+'
- 4-space indentation
- 100 character line limit
  (except for links and paths)
- No trailing whitespace characters
- No tabs (spaces only)
- No excessive blank lines at the end of files


Core Code Documentation Style
-----------------------------

- Semantic Linefeeds
- Doxygen comments with '///' and '///<'
- Comment ordering:

    #. description
    #. param
    #. returns
    #. pre
    #. post
    #. throws
    #. note
    #. warning
    #. todo

- Leave one Doxygen blank line between sections
- Always specify input and output parameters with @param[in,out]
- In-code TODOs with Doxygen '/// @todo'
  so that Doxygen picks them up.


GUI Code Documentation Style
----------------------------

- Semantic Linefeeds
- Leverage Qt Creator for auto-documentation
- Doxygen with C-style comments (default in Qt Creator)
- The same organization of Doxygen sections as in the core code.
