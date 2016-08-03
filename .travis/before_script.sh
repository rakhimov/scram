#!/usr/bin/env bash

set -ev

if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 20
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 20
  if [[ -z "${RELEASE}" && "$CXX" = "g++" ]]; then
    sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-4.9 20
  fi
fi

mkdir install
