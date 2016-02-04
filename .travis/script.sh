#!/usr/bin/env bash

set -ev
if [[ -n "${RELEASE}" ]]; then
  ./install.py -r --prefix=./install --threads 2
elif [[ "$CXX" = "g++" ]]; then
  ./install.py -p --prefix=./install --threads 2
else
  ./install.py -d --prefix=./install --threads 2
fi

./.travis/run_tests.sh

if [[ -z "${RELEASE}" && "$CXX" = "g++" ]]; then
  # Submit coverage of C++
  coveralls --exclude tests  --exclude ./build/CMakeFiles/ \
    --exclude ./build/lib/ --exclude ./build/gui/ \
    --exclude ./build/bin/  --exclude install/include/ \
    --exclude gui/ \
    --exclude src/error.cc \
    --exclude src/env.cc \
    --exclude src/version.cc \
    --exclude src/logger.cc \
    --exclude-pattern .*/src/.*\.h$ \
    > /dev/null

  # Submit coverage of Python
  codecov > /dev/null

  # Check documentation coverage
  doxygen ./.travis/doxygen.conf > /dev/null 2> doc_errors.txt
  # Deletion of compiler generated default functions
  sed -i '/=delete/d' doc_errors.txt
  # Doxygen 1.8.6 can't deal with C++11 initializer list in constructor.
  sed -i '/expression\.cc/d' doc_errors.txt
  if [[ -s doc_errors.txt ]]; then
    echo "Documentation errors:" >&2
    cat doc_errors.txt >&2
    exit 1
  fi

  # Lizard function complexity printout
  ( cd ./src && ../scripts/lizard_report.sh | tail -n 30 )
fi
