#!/usr/bin/env bash

set -ev

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  brew update
  # brew outdated boost || brew upgrade boost
  # brew install libxml2
  brew install gperftools
  brew install qt5
  brew install ccache
  # brew install python --universal
  sudo pip2 install nose
fi

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

pip install --user -U pip wheel

if [[ "${CONFIG}" == "Lint" ]]; then
  git clone https://github.com/Sarcasm/run-clang-format

  # Install newer doxygen due to bugs in 1.8.6 with C++11 code.
  DOXYGEN='doxygen-1.8.11.linux.bin.tar.gz'
  wget https://downloads.sourceforge.net/project/doxygen/rel-1.8.11/${DOXYGEN}
  tar -xf ${DOXYGEN}
  cp doxygen-1.8.11/bin/* ~/.local/bin/

  pip install --user -r requirements-dev.txt  # Python linting tools.

  exit 0  # Compilation dependencies are not required.
fi

pip install --user nose  # Testing main() requires nosetests!

# CMake 3.5
sudo apt-get install cmake3

if [[ "${CONFIG}" == "Coverage" ]]; then
  pip install --user -r requirements-dev.txt
  pip install --user -r requirements-tests.txt
fi
