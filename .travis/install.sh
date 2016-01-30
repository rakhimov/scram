#!/usr/bin/env bash

set -ev

sudo apt-get install -qq libboost-program-options-dev
sudo apt-get install -qq libboost-filesystem-dev
sudo apt-get install -qq libboost-system-dev
sudo apt-get install -qq libboost-math-dev
sudo apt-get install -qq libxml2-dev
sudo apt-get install -qq libxml++2.6-dev
sudo apt-get install -qq libgoogle-perftools-dev
sudo apt-get install -qq qt5-default
sudo apt-get install -qq python-lxml
sudo apt-get install -qq graphviz
sudo pip install nose

if [[ -z "${RELEASE}" && "$CXX" = "g++" ]]; then
  sudo apt-get install -qq doxygen
  sudo pip install cpp-coveralls
fi
