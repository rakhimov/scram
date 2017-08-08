#########################
Installation Instructions
#########################

.. note::
    Some SCRAM tools and tests may be absent in distribution packages.
    Also, the package version may be older than the latest release.


Debian
======

SCRAM is available in Debian’s official repositories since Debian Stretch.

.. code-block:: bash

    apt-get install scram

    apt-get install scram-gui  # The GUI front-end in unstable.


Ubuntu
======

SCRAM is available in Ubuntu's *universe* repository since Ubuntu 17.04.

.. code-block:: bash

    sudo apt-get install scram

Alternatively, run the following commands to get SCRAM from its PPA_.

.. code-block:: bash

    sudo add-apt-repository ppa:rakhimov/scram

    sudo apt-get update

    sudo apt-get install scram

    sudo apt-get install scram-gui  # The GUI front-end in PPA.

.. _PPA: https://launchpad.net/~rakhimov/+archive/ubuntu/scram


Fedora
======

Install from the official Fedora repositories:

.. code-block:: bash

    sudo dnf install scram


macOS
=====

Installation with homebrew_:

.. code-block:: bash

    brew install homebrew/science/scram

.. _homebrew: https://brew.sh/


Windows
=======

Download and run the installer_.

.. _installer: https://sourceforge.net/projects/iscram/files/latest/download


Docker Containers
=================

Available from Docker Hub: https://hub.docker.com/r/rakhimov/scram/


Other Platforms
===============

Please follow the building and installing instructions_ on GitHub.

.. _instructions: https://github.com/rakhimov/scram/tree/master#building-and-installing
