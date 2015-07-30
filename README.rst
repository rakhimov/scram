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
.. image:: https://badge.waffle.io/rakhimov/scram.png?label=ready&title=Ready
    :target: https://waffle.io/rakhimov/scram
    :alt: 'Stories in Ready'

**SCRAM** is a **C**\ommand-line **R**\isk **A**\nalysis **M**\ulti-tool.

This project aims to build a simple command line tool for probabilistic risk
analysis. SCRAM is currently capable of performing static fault tree analysis,
analysis with common cause failure models, probability calculations with
importance analysis, and uncertainty analysis with Monte Carlo simulations. This
tool can handle non-coherent fault trees, containing NOT logic.

SCRAM partially supports the OpenPSA_ model exchange format. SCRAM input and
report files are based on this format. For the current status of the OpenPSA MEF
features in SCRAM, please see the `MEF Support`_ documentation.

SCRAM generates a Graphviz Dot instruction file for a graphical representation
of a fault tree. An experimental GUI front-end is under development with Qt_ for
better visualization and manipulation of risk analysis models.

To explore the performance of SCRAM or research fault trees, a
fault tree generator script is provided, which can create hard-to-analyze fault
trees in a short time.

The documentation_ contains a full description of SCRAM, its current
capabilities, and future additions.

.. _OpenPSA: http://open-psa.org
.. _`MEF Support`: http://rakhimov.github.io/scram/doc/opsa_support.html
.. _documentation: http://rakhimov.github.io/scram
.. _Qt: http://qt-project.org/

To get SCRAM, you can download a virtual machine image on Sourceforge_ or follow
the building and installing instructions bellow.

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
`boost`                1.48.0
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
`TCMalloc`             1.7
====================   ==================


Optional Python modules:

====================   ==================
Package                Tested Version
====================   ==================
`argparse`             1.2.1
`nose`                 1.3.1
`lxml`                 3.3.3
====================   ==================


Compilers (Tested and Supported in CMake setup):

====================   ==================
Package                Tested Version
====================   ==================
`GCC/G++`              4.6, 4.8
`Clang/LLVM`           3.1
====================   ==================


Installing Dependencies (Linux and Unix)
========================================

The following installation instructions and scripts are taken from `Cyclus`_
project.

.. _Cyclus:
    https://github.com/cyclus/cyclus

This guide assumes that the user has root access (to issue sudo commands) and
access to a package manager or has some other suitable method of automatically
installing established libraries.


Linux Systems
-------------

This process is tested on `Travis CI`_ Ubuntu 12.04 LTS using apt-get as a
package manager.

The command to install a dependency takes the form of:

.. code-block:: bash

  sudo apt-get install package

Where "package" is replaced by the correct package name. The minimal list of
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
#. libgoogle-perftools-dev

compiler:

- gcc g++

For example, in order to install *graphviz* on your system, type:

.. code-block:: bash

    sudo apt-get install graphviz

If you'd prefer to copy/paste, the following line will install all major
SCRAM dependencies and GCC/G++ compiler:

.. code-block:: bash

    sudo apt-get install -y cmake make gcc g++ libboost-all-dev libboost-random-dev libxml2-dev libxml++2.6-dev python2.7 graphviz libgoogle-perftools-dev

For Ubuntu 12.04, the default Boost version is 1.46, so the update above version
1.47 is required:

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
for other systems as well.

Using macports_, the command to install a dependency takes the form of:

.. code-block:: bash

  sudo port install package

Where "package" is replaced by the correct package name. The minimal list of
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
SCRAM dependencies:

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

If not specified, the installation directory might be the user's *.local* or
*/usr/*. The default linkage is dynamic; however, tests are statically linked
against GoogleTest.

.. code-block:: bash

    .../scram$ python install.py  --prefix=path/to/installation/directory

The main and test binaries are installed in *installation/directory/bin*
directory. Also, the test input files and RelaxNG schema are copied in
*installation/directory/share/scram/*.

The default build type is Debug, but it can be overridden by "--release",
"--profile", or "--build-type" flags. For performance testing and distribution,
run the building with the release flag:

.. code-block:: bash

    .../scram$ python install.py --prefix=path/to/installation/directory -r

Various other flags are described by the script's help prompt.

.. code-block:: bash

    .../scram$ python install.py -h

Other tools, such as the fault tree generator and shorthand-to-XML converter,
can be found in the *scripts* directory. These tools do not need compilation or
installation.

The optional GUI front-end is built with Qt Creator and qmake.


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

To run the unit and benchmark tests:

.. code-block:: bash

    path/to/installation/directory/bin/scram_tests

To test the tools in the *scripts* directory:

.. code-block:: bash

    nosetests -w scripts/

To test the command-line call of SCRAM:

.. code-block:: bash

    nosetests -w tests/


To run performance tests
========================

A set of performance tests is provided to approximate the host's performance
in comparison to a reference computer with Ubuntu 14.04 with i5-2410M
processor. These tests can be helpful for developers to check for regressions.
More details can be found in performance test source files.

To run all performance tests (may take considerable time):

.. code-block:: bash

    path/to/installation/directory/bin/scram_tests --gtest_also_run_disabled_tests --gtest_filter=*Performance*


To run SCRAM
============

Example configuration and input files are provided in the *input* directory.

.. code-block:: bash

    path/to/installation/directory/bin/scram path/to/input/files


On command line, run help to get more detailed information:

.. code-block:: bash

    path/to/installation/directory/bin/scram --help

Various other tools, such as the fault tree generator and shorthand-to-XML
converter, can be found in the *scripts* directory. Help prompts and
documentation have more details how to use these tools.


**********************
Documentation Building
**********************

Documentation can be generated following the instructions in
the `gh-source`_ branch. The raw documentation files are in *doc* directory.

.. _`gh-source`:
    https://github.com/rakhimov/scram/tree/gh-source


**************
Note to a User
**************

The development may follow the Documentation Driven Development paradigm for
some new features. Therefore, some documentation may be ahead of the actual
development and describe features under current development or consideration.

For any questions, don't hesitate to ask the mailing list
(https://groups.google.com/forum/#!forum/scram-dev, scram-dev@googlegroups.com).


*****************
How to Contribute
*****************

Please follow instructions in `How to Contribute`_.

.. _`How to Contribute`:
    https://github.com/rakhimov/scram/blob/develop/CONTRIBUTING.md
