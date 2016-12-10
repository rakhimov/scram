SCRAM |release|
===============

**SCRAM** is a **C**\ommand-line **R**\isk **A**\nalysis **M**\ulti-tool.

SCRAM is a free and open source probabilistic risk analysis tool
that supports the Open-PSA_ `Model Exchange Format`_.

SCRAM is licensed under the GPLv3_.
The `release notes`_, `source code`_, and `issue/bug`_ tracker
are located at `GitHub <https://github.com/rakhimov/scram/tree/develop>`_.

.. rubric:: Code documentation:

- `C++ core code <api/index.xhtml>`_
- `Python scripts <scripts/modules.html>`_

.. _Open-PSA: http://open-psa.org
.. _Model Exchange Format: https://open-psa.github.io/mef
.. _GPLv3: https://github.com/rakhimov/scram/blob/master/LICENSE
.. _release notes: https://github.com/rakhimov/scram/releases
.. _source code: https://github.com/rakhimov/scram/tree/master
.. _issue/bug:  https://github.com/rakhimov/scram/issues


Implemented Features
--------------------

- Static fault tree analysis (MOCUS, BDD, ZBDD)
- Analysis of non-coherent fault trees (Minimal Cut Sets, Prime Implicants)
- Analysis with common-cause failure models
- Probability calculations with importance analysis
- Uncertainty analysis with Monte Carlo simulations
- Fault tree generator


Installation
------------

.. toctree::
    :maxdepth: 2

    doc/installation


Documentation
-------------

.. toctree::
    :caption: Development
    :maxdepth: 1

    doc/design_description
    doc/coding_standards
    doc/todo
    doc/bugs


.. toctree::
    :caption: Analysis
    :maxdepth: 1

    doc/description
    doc/fault_tree_analysis
    doc/fta_preprocessing
    doc/fta_algorithms
    doc/probability_analysis
    doc/uncertainty_analysis
    doc/common_cause_analysis


.. toctree::
    :caption: Format
    :maxdepth: 1

    doc/input_file
    doc/config_file
    doc/report_layer
    doc/opsa_support


.. toctree::
    :caption: Tools
    :maxdepth: 1

    doc/fault_tree_generator
    doc/gui


.. toctree::
    :caption: Miscellanea
    :maxdepth: 1

    doc/theory
    doc/references
    doc/citation
    doc/xml_comments


Contact Us
----------

- `User Support and Discussions`_
- `Announcement List`_
- `Developer Mailing List`_

.. _User Support and Discussions: https://groups.google.com/forum/#!forum/scram-users
.. _Announcement List: https://groups.google.com/forum/#!forum/scram-announce
.. _Developer Mailing List: https://groups.google.com/forum/#!forum/scram-dev
