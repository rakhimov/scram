##################################
Coding Style and Quality Assurance
##################################

This project adheres to the following coding styles
===================================================

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

Currently nonconforming cases in the C++ source code
----------------------------------------------------

* Exceptions are used. (This is not recommended by GCSG.)
* Streams are used instead of *printf-like routines*.
* Naming of mutator functions without *set* prefix.
* C++11 features are not allowed. Only Boost features are used instead.
* License boilerplate is not included unless the code is from other projects.

The quality of the code is checked with the following tools
===========================================================

#. Performance profiling with `Gprof`_.
#. Test coverage check with `Gcov`_ and reporting with `Coveralls`_.
#. Test status is tracked on `CDash`_.
#. Memory management bugs and leaks with `Valgrind`_.
#. Static code analysis with `Coverity`_.
#. Cyclomatic complexity analysis with `Lizard`_.
#. Google style conformance check with `Cpplint`_.
#. Common C++ code problem check with cppclean_.
#. Python code quality check with `Pylint`_.
#. Python code profiling with `PyVmMonitor`_.

.. _`Gprof`:
    https://www.cs.utah.edu/dept/old/texinfo/as/gprof.html
.. _`Gcov`:
    https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
.. _`Coveralls`:
    https://coveralls.io/r/rakhimov/scram
.. _`CDash`:
    http://my.cdash.org/index.php?project=SCRAM
.. _`Valgrind`:
    http://valgrind.org/
.. _`Coverity`:
    https://scan.coverity.com/projects/2555
.. _`Lizard`:
    https://github.com/terryyin/lizard
.. _`Cpplint`:
    https://google-styleguide.googlecode.com/svn/trunk/cpplint/
.. _cppclean:
    https://github.com/myint/cppclean
.. _`Pylint`:
    http://www.pylint.org/
.. _`PyVmMonitor`:
    http://www.pyvmmonitor.com/

Testing and Continuous Integration
==================================
In order to facilitate better software quality and quality assurance, full
test coverage is attempted through unit, integration, regression, and
benchmarking tests. The following tools are used for this purpose:

    - `GoogleTest`_
    - `Nose`_

These tests are automated, and continuous integration is provided by `Travis CI`_.

.. _`GoogleTest`:
    https://code.google.com/p/googletest/
.. _`Nose`:
    https://nose.readthedocs.org/en/latest/
.. _`Travis CI`:
    https://travis-ci.org/rakhimov/scram

Good references for testing and quality assurance
-------------------------------------------------

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

The project adheres to the Documentation Driven Development model (`DDD talk by Corey Oordt`_) following the best practices of `Agile Documentation`_ as well.

Documentation for the project is maintained in the reStructuredText_ format,
and the final representations are dynamically generated with Sphinx_ in various
formats (html, pdf, LaTeX).

Documentation for the code is checked and generated dynamically with Doxygen_.

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
