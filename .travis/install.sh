#!/usr/bin/env bash

set -ev

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  brew update
  brew outdated boost || brew upgrade boost
  brew install libxml++
  brew install gperftools
  brew install qt5
  brew install ccache
fi

sudo pip install -U pip wheel
sudo pip install -r requirements-tests.txt

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

if [[ "$CXX" == "icpc" ]]; then
  git clone https://github.com/nemequ/icc-travis.git
  ./icc-travis/install-icc.sh
fi

[[ -z "${RELEASE}" && "$CXX" = "g++" ]] || exit 0
sudo apt-get install -qq ggcov
sudo apt-get install -qq valgrind
sudo apt-get install -qq doxygen
sudo pip install -r requirements-dev.txt
