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

.. _Open-PSA: https://open-psa.github.io
.. _Model Exchange Format: https://open-psa.github.io/mef
.. _GPLv3: https://github.com/rakhimov/scram/blob/master/LICENSE
.. _release notes: https://github.com/rakhimov/scram/releases
.. _source code: https://github.com/rakhimov/scram/tree/master
.. _issue/bug:  https://github.com/rakhimov/scram/issues


Implemented Features
--------------------

- Event tree analysis (Fault tree linking, Event tree linking)
- Static fault tree analysis (MOCUS, BDD, ZBDD)
- Analysis of non-coherent fault trees (Minimal Cut Sets, Prime Implicants)
- Analysis with common-cause failure models
- Probability analysis (Importance factors, IEC 61508 Safety Integrity Levels)
- Uncertainty analysis with Monte Carlo simulations
- Alignment (Mission, Phases)
- Fault tree generator
- GUI front-end


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
    doc/event_tree_analysis
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
    doc/report_file
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


.. GitHub "Fork me" ribbon
.. raw:: html

    <a href="https://github.com/rakhimov/scram"><img style="position: absolute; top: 0; right: 0; border: 0;" src="https://camo.githubusercontent.com/a6677b08c955af8400f44c6298f40e7d19cc5b2d/68747470733a2f2f73332e616d617a6f6e6177732e636f6d2f6769746875622f726962626f6e732f666f726b6d655f72696768745f677261795f3664366436642e706e67" alt="Fork me on GitHub" data-canonical-src="https://s3.amazonaws.com/github/ribbons/forkme_right_gray_6d6d6d.png"></a>
