.. raw:: html

    <div style="text-align:center;"><br /><br />

.. image:: logo/scram_boiling.gif
    :align: center
    :alt: SCRAM

.. raw:: html

    </div>

SCRAM
=====

SCRAM: "Simplified Command-line Risk Analysis Multi-tool"

SCRAM is under development to be a free and open source
probabilistic risk analysis tool to perform fault tree analysis,
event tree analysis, uncertainty analysis, importance analysis,
common-cause analysis, and other probabilistic analysis types.

SCRAM is licensed under the GPLv3_.
The source code and issue/bug tracker are located at
`GitHub <https://github.com/rakhimov/scram>`_.

.. _GPLv3:
    https://github.com/rakhimov/scram/blob/master/LICENSE

Implemented Features
--------------------
- Static fault tree analysis
- Non-coherent analysis containing NOT logic or complements
- Analysis with common-cause failure models
- Probability calculations with importance analysis
- Uncertainty analysis with Monte Carlo simulations
- Fault tree generator
- Fault tree graphing with Graphviz Dot tool.
- OpenPSA Model Exchange Format

Installation
------------

.. toctree::
    :maxdepth: 2

    doc/installation.rst

Documentation
-------------

.. toctree::
    :maxdepth: 1

    doc/description
    doc/input_file
    doc/config_file
    doc/fta_implementation
    doc/fta_mcs_algorithm
    doc/probability_analysis
    doc/monte_carlo_simulations
    doc/common_cause_analysis
    doc/fta_graphing
    doc/gui
    doc/code_structure
    doc/coding_standards
    doc/bugs
    doc/todo
    doc/opsa_support
    doc/report_layer
    doc/fault_tree_generator
    doc/references
    doc/xml_comments

Theoretical Background
----------------------

.. toctree::
    :maxdepth: 2

    doc/theory.rst

Contact Us
----------

- `Mailing List and Forum`_

.. _`Mailing List and Forum`:
    https://groups.google.com/forum/#!forum/scram-dev
