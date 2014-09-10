#################################################
SCRAM Coding Style and Quality
#################################################

This project follows the following coding styles:
=================================================
#. `Google C++ Style Guide (GCSG)`_.
#. `Google Python Style Guide (GPSG)`_.
#. `PEP 8 -- Style Guide for Python Code (PEP8)`_.

.. _`Google C++ Style Guide (GCSG)`:
    http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml
.. _`Google Python Style Guide (GPSG)`:
    http://google-styleguide.googlecode.com/svn/trunk/pyguide.html
.. _`PEP 8 -- Style Guide for Python Code (PEP8)`:
    http://legacy.python.org/dev/peps/pep-0008/

Currently nonconforming cases in the source code:
-------------------------------------------------

* Exceptions are used. (This is not recommended by GCSG.)
* Functions use default values.
* Header guards do not include the source folder name.
* Null values for strings are not "\\0".
* Function output parameters are not pointers but references.
* Function input-then-output parameter ordering is not strict.

The quality of the code is checked by the following tools:
==========================================================
#. Performance Profiling with `Gprof`_.
#. Test coverage check with `Gcov`_ and reporting with `Coveralls`_.
#. Memory management bugs and leaks with `Valgrind`_.
#. Static code analysis with `Coverity`_.
#. Cyclomatic complexity analysis with `Lizard`_.
#. Google Style Conformance Check with `Cpplint`_.
#. Python code quality check with `Pylint`_.

.. _`Gprof`:
    https://www.cs.utah.edu/dept/old/texinfo/as/gprof.html
.. _`Gcov`:
    https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
.. _`Coveralls`:
    https://coveralls.io/r/rakhimov/SCRAM
.. _`Valgrind`:
    http://valgrind.org/
.. _`Coverity`:
    https://scan.coverity.com/projects/2555
.. _`Lizard`:
    https://github.com/terryyin/lizard
.. _`Cpplint`:
    https://google-styleguide.googlecode.com/svn/trunk/cpplint/
.. _`Pylint`:
    http://www.pylint.org/

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
    https://travis-ci.org/rakhimov/SCRAM

Good references for testing and quality information
---------------------------------------------------

- `Software Testing Fundamentals`_
- `Software Testing Tutorial`_
- `ISO Standards for Software Testing`_

.. _`Software Testing Fundamentals`:
    http://softwaretestingfundamentals.com/
.. _`Software Testing Tutorial`:
    http://www.tutorialspoint.com/software_testing/
.. _`ISO Standards for Software Testing`:
    http://softwaretestingstandard.org/
