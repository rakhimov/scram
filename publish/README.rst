#############
SCRAM Website
#############

The three branch system is used to build the website:

    **develop** -> **gh-source** -> **gh-pages**

The **gh-pages** is the final destination
of the generated files from **gh-source**.
This branch contains files for SCRAM website and API documentation.

The generated files are transferred from ``build/html``.
Before transferring the newly generated files,
remove all existing old files on this branch.
The ``publish.sh`` script can do this for you.

.. code-block:: bash

    bash build/html/publish.sh

The new changes must be amended
to the single commit on the **gh-pages** branch.
