#############
SCRAM Website
#############

The three branch system is used to build the website:

    **develop** -> **gh-source** -> **gh-pages**

The **gh-pages** is the final destination of the generated files from
**gh-source**. This branch contains files for SCRAM website and API
documentation.

The files are transfered from *build/publish*, *build/html*, and
*build/api/html*.

The *publis.sh* script is provided in *build/publish*, which does the steps
described bellow.

To run the script:

.. code-block:: bash

    bash build/publish/publish.sh

The example publishing steps:

    #. Clean previous web files that are going to be replaced.
    #. Clean all irrelevant files.
    #. Copy contents of *build/publish* into the root directory.
    #. Copy contents of *build/html* into the root directory.
    #. Make *api* directory.
    #. Copy contents of *build/api/html* into the *api/* directory.

The new changes must be amended to the single commit on the **gh-pages** branch.
