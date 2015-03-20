#####
SCRAM
#####

.. image:: https://travis-ci.org/rakhimov/scram.svg?branch=develop
    :target: https://travis-ci.org/rakhimov/scram
.. image:: https://coveralls.io/repos/rakhimov/scram/badge.png?branch=develop
    :target: https://coveralls.io/r/rakhimov/scram?branch=develop
.. image:: https://scan.coverity.com/projects/2555/badge.svg
    :target: https://scan.coverity.com/projects/2555
.. image:: doc/cdash.png
    :target: http://my.cdash.org/index.php?project=SCRAM

*SCRAM* : "Simplified Command-line Risk Analysis Multi-tool"

This project aims to build a simple command line tool for
probabilistic risk analysis. *SCRAM* is currently capable of performing
static fault tree analysis, analysis with common cause failure models,
probability calculations with importance analysis,
and uncertainty analysis using Monte Carlo simulations. This tool can handle
non-coherent fault trees, containing NOT logic.

*SCRAM* partially supports the OpenPSA_ model exchange format. Its input
files are based on this format.

*SCRAM* generates a Graphviz Dot instruction file for a graphical
representation of a fault tree.

An experimental GUI front-end is under development using `Qt`_.

In addition, a random fault tree generator script is provided to explore the
performance of *SCRAM*.

A full description of *SCRAM* and its current capabilities
is in `documentation`_.

.. _OpenPSA: http://open-psa.org
.. _`documentation`: http://rakhimov.github.io/scram
.. _`Qt`: http://qt-project.org/

To get *SCRAM*, you can download a virtual machine image
on Sourceforge_ or follow the building and installing instructions bellow.

.. _Sourceforge:
    https://sourceforge.net/projects/iscram/files/

.. contents:: **Table of Contents**
    :depth: 2

***********************
Building and Installing
***********************

A list of dependencies:

====================   ==================
Package                Minimum Version
====================   ==================
`CMake`                2.8
`boost`                1.47.0
`libxml2`              2
`libxml++`             2.34.1
`Python`               2.7.3
====================   ==================


Optional dependencies:

====================   ==================
Package                Minimum Version
====================   ==================
`Graphviz Dot`         2.26.3
`Qt`                   5.2.1
`Qt Creator`           3.0.1
====================   ==================


Compilers (Tested and Supported in CMake setup):

====================   ==================
Package                Tested Version
====================   ==================
`GCC/G++`              4.6.3, 4.8.2
`Clang/LLVM`           5.1
====================   ==================


Installing Dependencies (Linux and Unix)
========================================

The following installation instructions and scripts are taken from
`Cyclus`_ project.

.. _Cyclus:
    https://github.com/cyclus/cyclus

This guide assumes that the user has root access (to issue sudo commands) and
access to a package manager or has some other suitable method of automatically
installing established libraries.

Linux Systems
-------------

This process is tested using `Travis CI`_
Ubuntu 12.04 LTS using apt-get as a package manager.

The command to install a dependency takes the form of:

.. code-block:: bash

  sudo apt-get install package

where "package" is replaced by the correct package name. The minimal list of
required library package names is:

#. make
#. cmake
#. libboost-all-dev
#. libboost-random-dev
#. libxml2-dev
#. libxml++2.6-dev
#. python2.7

and (optionally):

#. graphviz
#. qt5-default
#. qtcreator

compiler:

- gcc g++

For example, in order to install *graphviz* on your system, type:

.. code-block:: bash

    sudo apt-get install graphviz

If you'd prefer to copy/paste, the following line will install all major
*SCRAM* dependencies and GCC/G++ compiler:

.. code-block:: bash

    sudo apt-get install -y cmake make gcc g++ libboost-all-dev libboost-random-dev libxml2-dev libxml++2.6-dev python2.7 graphviz

For Ubuntu 12.04, the default Boost version is 1.46, so the update above
version 1.47 is required:

.. code-block:: bash

    sudo apt-get install -y libboost-program-options1.48-dev libboost-random1.48-dev libboost-filesystem1.48-dev libboost-system1.48-dev

Some older systems may not have Qt 5 provided by default, so a workaround may
be needed. The optional installation for GUI:

.. code-block:: bash

    sudo apt-get install -y qt5-default qtcreator

.. _`Travis CI`:
    https://travis-ci.org/rakhimov/scram

Mac Systems
-----------

If on a Mac system, a good manager to use is macports_. It is assumed that
some dependencies are provided by Xcode, for example, *make*.
The following instructions are tested on OS X 10.9.2, but it should work
for other systems also.

Using macports_, the command to install a dependency takes the form of:

.. code-block:: bash

  sudo port install package

where "package" is replaced by the correct package name. The minimal list of
required library package names is:

#. cmake
#. boost
#. libxml2
#. libxmlxx2
#. python27

and (optionally):

#. graphviz
#. qt5-mac
#. qt5-creator-mac

compiler:

- clang/llvm

For example, in order to install *graphviz* on your system, type:

.. code-block:: bash

    sudo port install graphviz

If you'd prefer to copy/paste, the following line will install all major
*SCRAM* dependencies:

.. code-block:: bash

    sudo port install cmake boost libxml2 libxmlxx2 python27 graphviz


The optional installation for GUI building:

.. code-block:: bash

    sudo port install qt5-mac qt5-creator-mac

.. _macports:
    http://www.macports.org/

Installing SCRAM (Linux and Unix)
=================================

A python script is provided to make the installation process easier.
If there are dependency issues, the CMake output should guide with errors.
CMake can be used directly without the python script to configure the build.

There default installation directory is the user's
.local. The default linkage is dynamic; however, tests are statically linked
against GoogleTest.

.. code-block:: bash

    .../scram$ python install.py  --prefix=path/to/installation/directory

The main and test binaries are installed in *installation/directory/bin*
directory. Also, the test input files and RelaxNG schema are copied in
*installation/directory/share/scram/*.

For better performance run the building with the release flag:

.. code-block:: bash

    .../scram$ python install.py -r --prefix=path/to/installation/directory

Various other flags are described by the script's help prompt.

.. code-block:: bash

    .../scram$ python install.py -h

The optional GUI front-end is built using Qt Creator and qmake.

Windows
=======

Currently the easiest and best option is to use a virtual machine with
Ubuntu 14.04.

#. Install `VirtualBox <https://www.virtualbox.org/>`_
#. Get the system.

   a. Pre-configured image

        - Download `Ubuntu image with SCRAM`_ of the latest release version.
        - Open the downloaded .ova file with VirtualBox(File->Import Appliance)
        - The user name is 'scram'.
        - The password is 'scram'.

   b. New system.

        - Download `Ubuntu 14.04`_ or any other system.
        - Follow the installation instructions for Linux/Unix machines.

The other option is to use MinGW_, `Mingw-w64`_, or Cygwin_ and to build on
Windows.

Currently only Cygwin_ 64bit has been tested to produce binaries on Windows.
The dependencies listed for Linux systems must be installed with Cygwin64.
Unfortunately, this method requires building `libxml++`_ from source.

.. _`Ubuntu 14.04`:
    http://www.ubuntu.com/download
.. _MinGW:
    http://www.mingw.org/
.. _`Mingw-w64`:
    http://mingw-w64.sourceforge.net/
.. _Cygwin:
    https://www.cygwin.com/
.. _`libxml++`:
    http://libxmlplusplus.sourceforge.net/
.. _`Ubuntu image with SCRAM`:
    http://sourceforge.net/projects/iscram/files/ScramBox.ova/download

***********************
Running SCRAM and Tests
***********************

To run tests
=============

For dynamic builds (default):

.. code-block:: bash

    path/to/installation/directory/bin/scram_tests

For static builds (default Windows prepackages):

    #. Switch to the installation directory.
    #. Run the tests.

.. code-block:: bash

    cd path/to/installation/directory

.. code-block:: bash

    ./bin/scram_tests

.. note::
    For Windows, the test binary is **scram_tests.exe**

To run performance tests
========================

A set of performance tests is provided to approximate the host's performance
in comparison to a reference computer with Ubuntu 14.04 with i5-2410M
processor. These tests can be helpful for developers to check for regression.
More details can be found in performance test source files.

To run all the performance tests (may take considerable time):

.. code-block:: bash

    path/to/installation/directory/bin/scram_tests --gtest_also_run_disabled_tests --gtest_filter=*Performance*


To run SCRAM
============

On command line, run help to get running options:

.. code-block:: bash

    path/to/installation/directory/bin/scram -h

.. note::
    For Windows, the binary is **scram.exe**

More information about use and input files for SCRAM in `documentation`_.

**********************
Documentation Building
**********************

Documentation can be generated following the instruction in
the *gh-source* branch. The raw documentation files are in *doc/* directory.

**************
Note to a User
**************

The development follows the Documentation Driven Development paradigm.
Therefore, some documentation may be ahead of the actual development and
describe features under current development.

For any questions, don't hesitate to ask the mailing list (https://groups.google.com/forum/#!forum/scram-dev, scram-dev@googlegroups.com).

*****************
How to Contribute
*****************

Contributions are through `GitHub <https://github.com>`_ Pull Requests and
Issue Tracker.
Best practices are encouraged:

    - `Git SCM <http://git-scm.com/>`_
    - `Branching Model <http://nvie.com/posts/a-successful-git-branching-model/>`_
    - `Writing Good Commit Messages <https://github.com/erlang/otp/wiki/Writing-good-commit-messages>`_
    - `On Commit Messages <http://who-t.blogspot.com/2009/12/on-commit-messages.html>`_

`Coding Style and Quality`_

.. _`Coding Style and Quality`:
    https://rakhimov.github.io/scram/doc/coding_standards.html
