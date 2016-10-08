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

sudo -H pip install -U pip wheel
sudo -H pip install -r requirements-tests.txt

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

if [[ -n "${INTEL_COMPILER}" ]]; then
  git clone --branch icc17 https://github.com/rakhimov/icc-travis.git
  ./icc-travis/install-icc.sh
fi

# Installing dependencies from source.
PROJECT_DIR=$PWD
cd /tmp

# Boost
wget https://sourceforge.net/projects/iscram/files/deps/boost-scram.tar.bzip2
tar -xf ./boost-scram.tar.bzip2  # Sets up install dir.
sudo mv ./install/lib/* /usr/lib/
sudo mv ./install/include/boost /usr/include/

# Libxml++
LIBXMLPP='libxml++-2.38.1'
wget http://ftp.gnome.org/pub/GNOME/sources/libxml++/2.38/${LIBXMLPP}.tar.xz
tar -xf ${LIBXMLPP}.tar.xz
(cd ${LIBXMLPP} && ./configure && make -j 2 && sudo make install)

cd $PROJECT_DIR

[[ -z "${RELEASE}" && "$CXX" = "g++" ]] || exit 0

# Install newer doxygen due to bugs in 1.8.6 with C++11 code.
DOXYGEN='doxygen-1.8.12.linux.bin.tar.gz'
wget https://sourceforge.net/projects/iscram/files/deps/${DOXYGEN}
tar -xf ${DOXYGEN}
sudo cp doxygen-1.8.12/bin/* /usr/bin/

sudo apt-get install -qq ggcov
sudo apt-get install -qq valgrind
sudo -H pip install -r requirements-dev.txt
sudo apt-get install -qq lcov
gem install coveralls-lcov
