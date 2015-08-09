##################################
Coding Style and Quality Assurance
##################################

Coding Styles
=============

This project adheres to the following coding styles:

    #. `Google C++ Style Guide (GCSG)`_.
    #. `Google Python Style Guide (GPSG)`_.
    #. `PEP 8 -- Style Guide for Python Code (PEP8)`_.
    #. `KDE CMake Coding Style`_.
    #. `Qt Coding Style`_ for GUI.
    #. `Google Shell Style Guide`_.

.. _`Google C++ Style Guide (GCSG)`:
    http://google-styleguide.googlecode.com/svn/trunk/cppguide.html
.. _`Google Python Style Guide (GPSG)`:
    http://google-styleguide.googlecode.com/svn/trunk/pyguide.html
.. _`PEP 8 -- Style Guide for Python Code (PEP8)`:
    https://www.python.org/dev/peps/pep-0008/
.. _`KDE CMake Coding Style`:
    https://techbase.kde.org/Policies/CMake_Coding_Style
.. _`Qt Coding Style`:
    http://qt-project.org/wiki/Qt_Coding_Style
.. _`Google Shell Style Guide`:
    https://google-styleguide.googlecode.com/svn/trunk/shell.xml


Deviations from the GCSG
------------------------

- Exceptions are allowed
- Prefer streams to *printf-like routines*
- Name mutator functions without *set_* prefix
- Multiple inheritance is allowed
- C++11 features are currently not allowed. Only Boost features are used.


Monitoring Code Quality
=======================

C++
---

#. Performance profiling with Gprof_.
#. Code coverage check with Gcov_ and reporting with Coveralls_.
#. Test status is tracked on CDash_.
#. Memory management bugs and leaks with Valgrind_.
#. Static code analysis with Coverity_.
#. Cyclomatic complexity analysis with Lizard_.
#. Google style conformance check with Cpplint_.
#. Common C++ code problem check with cppclean_.

.. _Gprof:
    https://www.cs.utah.edu/dept/old/texinfo/as/gprof.html
.. _Gcov:
    https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
.. _Coveralls:
    https://coveralls.io/r/rakhimov/scram
.. _CDash:
    http://my.cdash.org/index.php?project=SCRAM
.. _Valgrind:
    http://valgrind.org/
.. _Coverity:
    https://scan.coverity.com/projects/2555
.. _Lizard:
    https://github.com/terryyin/lizard
.. _Cpplint:
    https://google-styleguide.googlecode.com/svn/trunk/cpplint/
.. _cppclean:
    https://github.com/myint/cppclean


Python
------

#. Code quality and style check with Pylint_.
#. Profiling with PyVmMonitor_.
#. Code coverage check with coverage_.

.. _Pylint:
    http://www.pylint.org/
.. _PyVmMonitor:
    http://www.pyvmmonitor.com/
.. _coverage:
    http://nedbatchelder.com/code/coverage/


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

In order to facilitate better software quality and quality assurance, full
test coverage is attempted through unit, integration, regression, and
benchmarking tests. The following tools are used for this purpose:

    - GoogleTest_
    - Nose_

These tests are automated, and continuous integration is provided by
`Travis CI`_.

.. _GoogleTest:
    https://code.google.com/p/googletest/
.. _Nose:
    https://nose.readthedocs.org/en/latest/
.. _`Travis CI`:
    https://travis-ci.org/rakhimov/scram


References for testing and quality assurance
--------------------------------------------

- `Software Testing Fundamentals`_
- `Software Testing Tutorial`_
- `ISO Standards for Software Testing`_
- `Introduction to Test Driven Development`_

.. _`Software Testing Fundamentals`:
    http://softwaretestingfundamentals.com/
.. _`Software Testing Tutorial`:
    http://www.tutorialspoint.com/software_testing/
.. _`ISO Standards for Software Testing`:
    http://softwaretestingstandard.org/
.. _`Introduction to Test Driven Development`:
    http://agiledata.org/essays/tdd.html


Version control and Versioning
==============================

- `Git SCM`_
- `Branching Model`_
- `Writing Good Commit Messages`_
- `On Commit Messages`_
- `Semantic Versioning`_

.. _`Git SCM`:
    http://git-scm.com/
.. _`Branching Model`:
    http://nvie.com/posts/a-successful-git-branching-model/
.. _`Writing Good Commit Messages`:
    https://github.com/erlang/otp/wiki/Writing-good-commit-messages
.. _`On Commit Messages`:
    http://who-t.blogspot.com/2009/12/on-commit-messages.html
.. _`Semantic Versioning`:
    http://semver.org/


Documentation
=============

Good documentation of the code and functionality is the requirement for
maintainability and evolution of the project and its acceptance by users.

The project adheres to the Documentation Driven Development model
(`DDD talk by Corey Oordt`_), following the best practices of
`Agile Documentation`_ as well.

The documentation for the project is maintained in the reStructuredText_ format,
and the final representations are dynamically generated with Sphinx_ in various
formats (html, pdf, LaTeX).

The documentation for the code is checked and generated dynamically with
Doxygen_.

The source text of the documentation in the code and the reST format
must be formatted consistently and with `Semantic Linefeeds`_
for maintainability and version control.

.. _Doxygen:
    http://doxygen.org/
.. _Sphinx:
    http://sphinx-doc.org/
.. _reStructuredText:
    http://docutils.sourceforge.net/rst.html
.. _`DDD talk by Corey Oordt`:
    http://pyvideo.org/video/441/pycon-2011--documentation-driven-development
.. _`Agile Documentation`:
    http://www.agilemodeling.com/essays/agileDocumentationBestPractices.htm
.. _Semantic Linefeeds:
    http://rhodesmill.org/brandon/2012/one-sentence-per-line/
