####################
SCRAM Website Source
####################

.. image:: https://travis-ci.org/rakhimov/scram.svg?branch=gh-source
    :target: https://travis-ci.org/rakhimov/scram

|

The three branch system is used to build the website:

    **develop** -> **gh-source** -> **gh-pages**

This branch contains source files
for building SCRAM's website and API documentation.
This **gh-source** branch is synchronized with the **develop** branch
to get the current documentation files and source code of SCRAM.
The generated website is automatically deployed to **gh-pages** branch by Travis-CI.


********
Building
********

================   =================
Package            Minimum Version
================   =================
Sphinx             1.4.3
sphinx_rtd_theme   0.1.8
LaTeX
doxygen            1.8.12
Graphviz Dot       2.26.3
cppdep             0.2.2
================   =================

Update and configure ``source/conf.py`` as necessary.
Configure ``doxygen.conf`` if needed.
The command to build the API documentation.

.. code-block:: bash

    make doxygen

The command to build the website.

.. code-block:: bash

    make html

The results are located in ``build/html``.
