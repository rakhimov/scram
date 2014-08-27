###########
SCRAM
###########

.. image:: https://travis-ci.org/rakhimov/SCRAM.svg?branch=develop
    :target: https://travis-ci.org/rakhimov/SCRAM
.. image:: https://coveralls.io/repos/rakhimov/SCRAM/badge.png?branch=develop
    :target: https://coveralls.io/r/rakhimov/SCRAM?branch=develop
.. image:: https://scan.coverity.com/projects/2555/badge.svg
    :target: https://scan.coverity.com/projects/2555

*SCRAM* : "Simplified Command-line Risk Analysis Multi-tool"

This project attempts to build a simple command line tool for risk analysis.
Static Fault Tree Analysis is implemented.
In addition, a random fault tree generator is
provided to explore the performance of *SCRAM*. A fault tree can be drawn by
Graphviz Dot tool.

A full description of *SCRAM* and its current capabilities
is in `the documentation`_.

.. _`the documentation`: http://rakhimov.github.io/SCRAM

******************************
Building and Installing
******************************

A list of dependencies:

====================   ==================
Package                Minimum Version
====================   ==================
`CMake`                2.8
`boost`                1.46.1
`libxml2`              2
`libxml++`             2.34.1
`Python`               2.7.3
====================   ==================


Optional dependencies:

====================   ==================
Package                Minimum Version
====================   ==================
`Graphviz Dot`         2.26.3
====================   ==================

Installing Dependencies (Linux and Unix)
========================================

The following installation instructions and scripts are taken from
`Cyclus`_ project.

.. _Cyclus:
    https://github.com/cyclus/cyclus

This guide assumes that the user has root access (to issue sudo commands) and
access to a package manager or has some other suitable method of automatically
installing established libraries. This process is tested using `Travis CI`_
Ubuntu 12.04 LTS using apt-get as the package manager;
if on a Mac system, a good manager to use is macports_.
In that case, replace all of the following instances of "apt-get" with "port".

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

For example, in order to install *graphviz* on your system, type:

.. code-block:: bash

  sudo apt-get install graphviz

If you'd prefer to copy/paste, the following line will install all *SCRAM*
dependencies:

.. code-block:: bash

   sudo apt-get install -y cmake make libboost-all-dev libboost-random-dev libxml2-dev libxml++2.6-dev python2.7 graphviz

.. _`Travis CI`:
    https://travis-ci.org/rakhimov/SCRAM
.. _macports:
    http://www.macports.org/

Installing SCRAM (Linux and Unix)
=================================

A python script is provided to make the installation process easier.
If there are dependency issues, the CMake output should guide with errors.

The default build is DEBUG. There default installation directory is the user's
.local.

.. code-block:: bash

    .../scram$ python install.py  --prefix=path/to/installation/directory

The executable test binary is installed in *installation/directory/bin* directory.
Also, the test input files and RelaxNG schema are copied in *installation/directory/share/scram/*.
In order to run tests:

.. code-block:: bash

    .../scram$ path/to/installation/directory/bin/scram_unit_tests

For better performance run the building with the optimization flag:

.. code-block:: bash

    .../scram$ python install.py -o --prefix=path/to/installation/directory

Various other flags are described by the script's help prompt.

.. code-block:: bash

    .../scram$ python install.py -h

Windows
=======

Currently the easiest option is to use a virtual machine with `Ubuntu 14.04`_.

#. Install `VirtualBox <https://www.virtualbox.org/>`_
#. Download `Ubuntu 14.04`_
#. Follow the installation instructions for Linux machines.

The other option is to use MinGW_ or Cygwin_ and to build on Windows machine,
but this option is not yet tested.

.. _`Ubuntu 14.04`:
    http://www.ubuntu.com/download
.. _MinGW:
    http://www.mingw.org/
.. _Cygwin:
    https://www.cygwin.com/

*****************************
Note to a User
*****************************

The development follows the Documentation Driven Development paradigm.
Therefore, some documentation may refer to not yet developed features or the
features under current development.
