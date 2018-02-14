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
  sudo pip2 install pytest
fi

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

pip install --user -U pip wheel

if [[ "${CONFIG}" == "Lint" ]]; then
  git clone https://github.com/Sarcasm/run-clang-format
  pip install --user -r requirements-dev.txt  # Python linting tools.
  exit 0  # Compilation dependencies are not required.
fi

pip install --user pytest  # Testing main() requires pytest!

if [[ "${CONFIG}" == "Coverage" ]]; then
  pip install --user -r requirements-dev.txt
  pip install --user -r requirements-tests.txt
fi
