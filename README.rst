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

*****************************
Note to a User
*****************************

The development follows the Documentation Driven Development paradigm.
Therefore, some documentation may refer to not yet developed features or the
features under current development.
