#!/usr/bin/env bash

set -ev

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  brew install libxml++
  brew install gperftools
  brew install qt5
  brew install graphviz
  brew install python
fi

sudo pip install nose
sudo pip install lxml

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0
[[ -z "${RELEASE}" && "$CXX" = "g++" ]] || exit 0
sudo apt-get install -qq valgrind
sudo apt-get install -qq doxygen
sudo pip install cpp-coveralls
sudo pip install lizard
sudo pip install codecov
sudo pip install coverage
sudo pip install cpplint
