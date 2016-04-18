#!/usr/bin/env bash

set -ev

sudo pip install nose

[[ -z "${RELEASE}" && "$CXX" = "g++" ]] || exit 0
sudo apt-get install -qq valgrind
sudo apt-get install -qq doxygen
sudo pip install cpp-coveralls
sudo pip install lizard
sudo pip install codecov
sudo pip install coverage
sudo pip install cpplint
