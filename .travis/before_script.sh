#!/usr/bin/env bash

set -ev

mkdir build

[[ "${CONFIG}" != "Lint" ]] || exit 0

mkdir install

if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 20
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 20
  if [[ "${CONFIG}" == "Coverage" ]]; then
    sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-4.9 20
  fi
fi
