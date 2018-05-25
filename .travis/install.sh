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
  pip install pytest
fi

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

pip install -U pip wheel

if [[ "${CONFIG}" == "Lint" ]]; then
  git clone https://github.com/Sarcasm/run-clang-format
  pip install -r requirements-dev.txt  # Python linting tools.
  exit 0  # Compilation dependencies are not required.
fi

pip install pytest  # Testing main() requires pytest!

if [[ "${CONFIG}" == "Coverage" ]]; then
  pip install -r requirements-dev.txt
  pip install -r requirements-tests.txt
fi
