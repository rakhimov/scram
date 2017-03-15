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
SCRAM is capable of performing static fault tree analysis,
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
    :depth: 2


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

====================   ==================
Package                Minimum Version
====================   ==================
CMake                  2.8.12
boost                  1.56
libxml++               2.38.1
Python                 2.7.3 or 3.3
Qt                     5.2.1
====================   ==================


Optional Dependencies
---------------------

====================   ==================
Package                Minimum Version
====================   ==================
TCMalloc               1.7
JEMalloc               3.6
====================   ==================


Compilers
---------

====================   ==================
Package                Minimum Version
====================   ==================
GCC/G++                4.9
Clang/LLVM             3.6
Intel                  17.0.1
====================   ==================


Installing Dependencies
=======================

The following installation instructions and scripts are taken from Cyclus_.

.. _Cyclus: https://github.com/cyclus/cyclus

This guide assumes that the user has root access (to issue sudo commands)
and access to a package manager
or has some other suitable method of automatically installing libraries.


Ubuntu
------

This process is tested on Ubuntu 16.04 LTS
using ``apt-get`` as the package manager.

The command to install a dependency takes the form of:

.. code-block:: bash

    sudo apt-get install package

Where ``package`` is replaced by the correct package name.
The minimal list of required library package names is:

#. cmake
#. libboost-all-dev
#. libxml++2.6-dev
#. qt5-default

and (optionally):

#. libgoogle-perftools-dev

compiler:

- gcc g++

For example, in order to install ``Boost`` on your system, type:

.. code-block:: bash

    sudo apt-get install libboost-all-dev

Python and GCC/G++ compilers are assumed to be available on the system.
If you'd prefer to copy/paste,
the following line will install all major dependencies:

.. code-block:: bash

    sudo apt-get install -y cmake lib{boost-all,xml++2.6,google-perftools}-dev qt5-default


macOS
-----

If on a Mac system, a good manager to use is macports_ or homebrew_.
It is assumed that some dependencies are provided by Xcode.
The following instructions are tested on OS X 10.9,
but it should work for later versions as well.

Using homebrew_, the command to install a dependency takes the form of:

.. code-block:: bash

    brew install package

If the ``package`` is already installed the command will fail,
instead upgrade the ``package`` if necessary:

.. code-block:: bash

    brew outdated package || brew upgrade package

Where ``package`` is replaced by the correct package name.
The minimal list of required library package names is:

#. cmake
#. boost
#. libxml++
#. qt5

and (optionally):

#. gperftools

compiler:

- clang/llvm

For example, in order to install ``Boost`` on your system, type:

.. code-block:: bash

    brew install boost

If you'd prefer to copy/paste,
the following line will install all major dependencies:

.. code-block:: bash

    brew install cmake boost libxml++ gperftools qt5

.. _macports: http://www.macports.org/
.. _homebrew: http://brew.sh/


Windows
-------

MSYS2_/Mingw-w64_ is the recommended platform to work on Windows.
Assuming MSYS2 is installed on the system,
the following instructions will install SCRAM dependencies.

Using ``pacman``, in MSYS2_64 command shell,
a C++ dependency installation takes the form of:

.. code-block:: bash

    pacman -S mingw-w64-x86_64-package

Where ``package`` is replaced by the correct package name:

#. gcc
#. make
#. cmake
#. boost
#. libxml++2.6
#. qt5

and (optionally):

#. jemalloc

If Python has not yet been installed on the system,
Python installation takes the form of:

.. code-block:: bash

    pacman -S python

If you'd prefer to copy/paste,
the following line will install all major dependencies:

.. code-block:: bash

    pacman --noconfirm -S python mingw-w64-x86_64-{gcc,make,cmake,boost,libxml++2.6,qt5,jemalloc}

SCRAM installation and executables must be run inside of the MSYS2 shell.

.. _MSYS2: https://sourceforge.net/projects/msys2/
.. _Mingw-w64: http://mingw-w64.sourceforge.net/


Installing SCRAM
================

A python script is provided to make the build/installation process easier.
If there are dependency issues, ``CMake`` output should guide with errors.
``CMake`` can be used directly without the python script to configure the build.

The default installation directory is ``~/.local``.
The default linkage is dynamic;
however, tests are statically linked against GoogleTest.

.. code-block:: bash

    .../scram$ python install.py  --prefix=path/to/installation/directory

The main and test binaries are installed in ``installation/directory/bin``.
The input files and schema are copied in ``installation/directory/share/scram/``.

The default build type is ``Debug`` with many compiler warnings turned on,
but it can be overridden by ``--release`` or ``--build-type CMAKE_BUILD_TYPE``.
For performance testing and distribution, use ``--release`` flag:

.. code-block:: bash

    .../scram$ python install.py --prefix=path/to/installation/directory --release

For Mingw-w64_ on Windows, add ``--mingw64`` flag.

.. code-block:: bash

    .../scram$ python install.py --prefix=path/to/installation/directory --release --mingw64

Various other flags are described by the script's help prompt.

.. code-block:: bash

    .../scram$ python install.py --help

Other tools, such as the **fault tree generator**,
can be found in the ``scripts`` directory.
These tools do not need compilation or installation.


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
