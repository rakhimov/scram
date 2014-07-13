#################################################
SCRAM Coding Style
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
=================================================

* Exceptions are used. (This is not recommended by GCSG)
* Functions use default values.
* Header guards do not include the source file name.
* Null values for strings are not "\0".
* Some function names are not camel case.
