#!/usr/bin/env bash

set -ev

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  brew update
  # brew outdated boost || brew upgrade boost
  brew install libxml2
  brew install gperftools
  brew install qt5
  brew install ccache
fi

sudo -H pip install -U pip wheel
sudo -H pip install nose  # Testing main() requires nosetests!

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

if [[ "$CXX" = "clang++" ]]; then
  sudo apt-get install -qq clang-${CLANG_VERSION}
  if [[ -z "${RELEASE}" ]]; then
    sudo apt-get install -qq clang-format-${CLANG_VERSION}
    git clone https://github.com/Sarcasm/run-clang-format
  fi
fi
# Boost
sudo apt-get install \
  libboost-{program-options,math,random,filesystem,system,date-time}1.58-dev

[[ -z "${RELEASE}" && "$CXX" = "g++" ]] || exit 0

# Install newer doxygen due to bugs in 1.8.6 with C++11 code.
DOXYGEN='doxygen-1.8.11.linux.bin.tar.gz'
wget https://downloads.sourceforge.net/project/doxygen/rel-1.8.11/${DOXYGEN}
tar -xf ${DOXYGEN}
sudo cp doxygen-1.8.11/bin/* /usr/bin/

sudo apt-get install -qq ggcov
sudo apt-get install -qq valgrind
sudo apt-get install -qq lcov

sudo -H pip install -r requirements-dev.txt
sudo -H pip install -r requirements-tests.txt
