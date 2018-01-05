#####
SCRAM
#####

.. image:: https://travis-ci.org/rakhimov/scram.svg?branch=develop
    :target: https://travis-ci.org/rakhimov/scram
.. image:: https://ci.appveyor.com/api/projects/status/d36yu2w3t8hy4ito/branch/develop?svg=true
    :target: https://ci.appveyor.com/project/rakhimov/scram/branch/develop
    :alt: 'Build status'
.. image:: https://codecov.io/github/rakhimov/scram/coverage.svg?branch=develop
    :target: https://codecov.io/github/rakhimov/scram?branch=develop
.. image:: https://scan.coverity.com/projects/2555/badge.svg
    :target: https://scan.coverity.com/projects/2555
.. image:: https://landscape.io/github/rakhimov/scram/develop/landscape.svg?style=flat
    :target: https://landscape.io/github/rakhimov/scram/develop
    :alt: Code Health
.. image:: https://badge.waffle.io/rakhimov/scram.svg?label=In%20Progress&title=in%20progress
    :target: https://waffle.io/rakhimov/scram
    :alt: 'Stories in Progress'

|

**SCRAM** is a **C**\ommand-line **R**\isk **A**\nalysis **M**\ulti-tool.

This project aims to build a command line tool for probabilistic risk analysis.
SCRAM is capable of performing event tree analysis, static fault tree analysis,
analysis with common cause failure models,
probability calculations with importance analysis,
and uncertainty analysis with Monte Carlo simulations.
This tool can handle non-coherent fault trees, containing NOT logic.

SCRAM input and report files are based on the Open-PSA_ `Model Exchange Format`_.
For the current status of the Open-PSA MEF features in SCRAM,
please see the `MEF Support`_ documentation.

A complementary GUI front-end is under development
for visualization and manipulation of risk analysis models and reports.

To explore the performance of SCRAM or research fault trees,
a fault tree generator script is provided,
which can create hard-to-analyze fault trees in a short time.

The documentation_ contains a full description of SCRAM,
its current capabilities, and future additions.
The latest stable release is packaged for `quick installation`_ on various platforms.

.. _Open-PSA: http://open-psa.org
.. _Model Exchange Format: http://open-psa.github.io/mef
.. _MEF Support: https://scram-pra.org/doc/opsa_support.html
.. _documentation: https://scram-pra.org
.. _quick installation: https://scram-pra.org/doc/installation.html

.. contents:: **Table of Contents**


***********************
Building and Installing
***********************

Git Submodules
==============

Some dependencies are provided with git submodules (e.g., Google Test).
In order to initialize all the submodules,
this repository must be cloned recursively with ``git clone --recursive``,
or the following commands must be executed after a normal clone.

.. code-block:: bash

    git submodule update --init --recursive


Dependencies
============

====================   ===============
Package                Minimum Version
====================   ===============
CMake                  3.8
boost                  1.61
libxml2                2.9.1
Python                 2.7.3 or 3.3
Qt                     5.2.1
====================   ===============


Optional Dependencies
---------------------

====================   ===============
Package                Minimum Version
====================   ===============
TCMalloc               1.7
JEMalloc               3.6
====================   ===============


Compilers
---------

====================   ===============
Package                Minimum Version
====================   ===============
GCC/G++                7.1
Clang/LLVM             5.0
Intel                  18.0.1
====================   ===============


Installing Dependencies
=======================

Ubuntu
------

Python and GCC/G++ compiler are assumed to be available on the system.
The process is tested on Ubuntu 17.10 using ``apt-get`` as the package manager:

.. code-block:: bash

    sudo apt-get install -y cmake lib{boost-all,xml2,google-perftools,qt5{svg,opengl}5}-dev qt{base,tools}5-dev{,-tools}


macOS
-----

If on a Mac system, homebrew_ is a good package manager to use.
It is assumed that some dependencies are provided by Xcode (e.g., Python, llvm/clang, make).
The following instructions are tested on OS X 10.12:

.. code-block:: bash

    brew install cmake boost libxml2 gperftools qt5

.. _homebrew: http://brew.sh/


Windows
-------

MSYS2_/Mingw-w64_ is the recommended platform to work on Windows.
Assuming MSYS2 is installed on the system,
the following instructions will install SCRAM dependencies:

.. code-block:: bash

    pacman --noconfirm -S mingw-w64-x86_64-{gcc,make,cmake,boost,libxml2,qt5,jemalloc}

SCRAM installation and executables must be run inside of the MSYS2 shell.

.. _MSYS2: https://sourceforge.net/projects/msys2/
.. _Mingw-w64: http://mingw-w64.sourceforge.net/


Installing SCRAM
================

The project is configured with CMake_ scripts.
CMake generates native "makefiles" or build system configurations
to be used in your compiler environment.
If there are dependency issues, CMake output should guide with errors.
The configuration and build must happen out-of-source (e.g., in ``build`` sub-directory).

.. code-block:: bash

    .../scram/build$ cmake .. -DCMAKE_INSTALL_PREFIX=path/to/installation/directory -DCMAKE_BUILD_TYPE=Release

For Mingw-w64_ on Windows, specify ``-G "MSYS Makefiles"`` generator flag.
To build tests, specify ``-DBUILD_TESTING=ON`` option.

Various other project configurations can be explored with CMake or its front-ends.
For example:

.. code-block:: bash

    .../scram/build$ cmake -L

    .../scram/build$ ccmake .

    .../scram/build$ cmake-gui .

An example build/install instruction with the CMake generated Makefiles:

.. code-block:: bash

    .../scram/build$ make install

The main and test binaries are installed in ``installation/directory/bin``.
The input files and schema are copied in ``installation/directory/share/scram/``.

Other tools, such as the **fault tree generator**,
can be found in the ``scripts`` directory.
These tools do not require compilation or installation.

.. _CMake: https://cmake.org


***********************
Running SCRAM and Tests
***********************

This guide assumes
that SCRAM *installation* directories are in the global path.
If this is not the case,
``path/to/installation/directory/bin/`` must be prepended to the command-line calls.
However, if SCRAM executables are not in the path,
some system tests and scripts cannot be initiated.


To run SCRAM
============

Example configuration and input files are provided in the ``input`` directory.

.. code-block:: bash

    scram path/to/input/files


On command line, run help to get more detailed information:

.. code-block:: bash

    scram --help

Various other useful tools and helper scripts,
such as the **fault tree generator**,
can be found in the ``scripts`` directory.
Help prompts and the documentation have more details how to use these tools.


To launch the GUI
=================

To launch the GUI front-end from the command-line:

.. code-block:: bash

    scram-gui

The command can also take project configuration and/or input files:

.. code-block:: bash

    scram-gui path/to/input/files

    scram-gui --config-file path/to/config/file

    scram-gui path/to/input/files --config-file path/to/config/file


To run tests
============

To run the unit and benchmark tests:

.. code-block:: bash

    scram_tests

To test the tools in the ``scripts`` directory:

.. code-block:: bash

    nosetests -w scripts/ test/

To test the command-line call of SCRAM:

.. code-block:: bash

    nosetests -w tests/


To run performance tests
========================

A set of performance tests is provided
to evaluate the running times on the host machine
and to help developers check for regressions.
More details can be found in performance test source files.

To run all performance tests (may take considerable time):

.. code-block:: bash

    scram_tests --gtest_also_run_disabled_tests --gtest_filter=*Performance*


To run GUI tests
================

Unfortunately, Qt Test does not automatically register or manage all its test cases,
nor does it provide a single test driver.
Each test case is a separate binary with its own commands and reports.
Take a look at ``path/to/installation/directory/bin`` directory
for the compiled ``scramgui_test${CASE_NAME}`` binaries to run.

All Qt Tests are also manually registered with CTest
so that it is possible to run all the GUI tests at once:

.. code-block:: bash

    .../scram/build$ ctest --verbose


To run fuzz testing
===================

The main goal of SCRAM fuzz testing
is to discover defects in its analysis code.
It is recommended to build SCRAM
with assertions preserved
and sanitizers enabled, for example,
address sanitizer in GCC and Clang ``-fsanitize=address``.

In order to speed up the fuzz testing,
SCRAM may be built with optimizations but ``NDEBUG`` undefined.
Additionally, multiple SCRAM instances can be run at once.

An example command to run SCRAM 1000 times with 4 parallel instances:

.. code-block:: bash

    fuzz_tester.py -n 1000 -j 4

The fuzz tester can be guided with options listed in its help prompt.
Some options can be combined,
and some are mutually exclusive.
The priorities of mutually exclusive options and combinations are hard-coded in the script,
and no error messages are produced;
however, information messages are given to indicate the interpretation.

.. code-block:: bash

    fuzz_tester.py --help

Fuzzing inputs and configurations are auto-generated.
The fuzz tester collects run configurations, failures, and logs.
The auto-generated inputs are preserved for failed runs.


Cross Validation
----------------

The Fuzz tester can check
the results of qualitative analysis algorithms implemented in SCRAM.
If there is any disagreement between various algorithms,
the run is reported as failure.

.. code-block:: bash

    fuzz_tester.py --cross-validate


**********************
Documentation Building
**********************

Documentation is generated with the configurations on the gh-source_ branch.
The raw documentation files are in the ``doc`` directory.

.. _gh-source: https://github.com/rakhimov/scram/tree/gh-source


**************
Note to a User
**************

The development may follow
the Documentation Driven Development paradigm for some new features.
Therefore, some documentation may be ahead of the actual development
and describe features under current development or consideration.

For any questions, don't hesitate to ask the user support mailing list
(https://groups.google.com/forum/#!forum/scram-users, scram-users@googlegroups.com).

For latest releases and information about SCRAM,
feel free to subscribe to the announcements
(https://groups.google.com/forum/#!forum/scram-announce,
scram-announce+subscribe@googlegroups.com).


*****************
How to Contribute
*****************

Please follow the instructions in `CONTRIBUTING.md`_.

.. _CONTRIBUTING.md:
    https://github.com/rakhimov/scram/blob/develop/CONTRIBUTING.md


.. image:: https://bestpractices.coreinfrastructure.org/projects/356/badge
    :target: https://bestpractices.coreinfrastructure.org/projects/356
    :alt: CII Best Practices
.. image:: https://www.openhub.net/p/scram/widgets/project_thin_badge.gif
    :target: https://www.openhub.net/p/scram
    :alt: Open HUB Metrics
.. image:: https://d322cqt584bo4o.cloudfront.net/scram/localized.svg
    :target: https://crowdin.com/project/scram
    :alt: Crowdin
.. image:: https://zenodo.org/badge/17964226.svg
    :target: https://zenodo.org/badge/latestdoi/17964226
    :alt: Zenodo DOI
