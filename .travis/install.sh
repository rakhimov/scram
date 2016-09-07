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

# Installing dependencies from source.
PROJECT_DIR=$PWD
mkdir deps
cd deps
mkdir install
INSTALL_PATH=$PWD/install
export PATH=$INSTALL_PATH/include:$INSTALL_PATH/lib:$PATH

wget https://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.bz2
tar --bzip2 -xf boost_1_56_0.tar.bz2
cd boost_1_56_0
./bootstrap.sh --prefix=$INSTALL_PATH --with-libraries=math,filesystem,program_options,system,date_time
./b2 link=shared threading=single variant=release -j2 -q install
cd $PROJECT_DIR

[[ -z "${RELEASE}" && "$CXX" = "g++" ]] || exit 0
sudo apt-get install -qq ggcov
sudo apt-get install -qq valgrind
sudo apt-get install -qq doxygen
sudo pip install -r requirements-dev.txt
