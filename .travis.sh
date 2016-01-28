#!/usr/bin/env bash

set -ev
if [[ "$CXX" = "g++" ]]; then
  ./install.py -p --prefix=./install --threads 2
else
  ./install.py -d --prefix=./install --threads 2
fi

./.run_tests.sh

if [[ "$CXX" = "g++" ]]; then
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
fi
