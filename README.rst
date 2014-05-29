###########
SCRAM
###########

SCRAM : "Simplified Command-line Risk Analysis Multi-tool"

This project attempts to build a simple command line tool for risk analysis.
Fault Tree Analysis is implemented. In addition, a random tree generator is
provided for exploring performance of SCRAM. A fault tree can be drawn by
Graphviz Dot tool.

A full description of SCRAM and its capabilities is in documentation directory.

******************************
Building and Installing
******************************

A list of dependencies:

====================   ==================
Package                Minimum Version
====================   ==================
`CMake`                2.8
`boost`                1.46.1
====================   ==================


Optional dependencies:

====================   ==================
Package                Minimum Version
====================   ==================
`Python`               2.7.4
`Graphviz Dot`         2.26.3
====================   ==================

Building SCRAM (Linux and Unix)
===============================

Currently only building is enough to get SCRAM working for testing purposes.
A python script is provided to make the process easier.
After installing the dependencies, run the following inside main SCRAM directory:

.. code-block:: bash

    .../scram$ python install.py

Executable file and tests will be located in build/bin directory.

For better performance run the building with the optimization flag:

.. code-block:: bash

    .../scram$ python install.py -o

Various other flags are described by the script's help prompt.

*****************************
Note to a User
*****************************

The development follows the Documentation Driven Development paradigm.
Therefore, some documentation may refer to not yet developed features or the
features under current development.
