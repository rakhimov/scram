#!/usr/bin/env bash

set -ev

# Assuming that path contains the built binaries.
# And this script is called from the root directory.
which scram
which scram_tests
which scram-gui

scram_tests
nosetests -w ./tests/
(cd build && ctest --verbose)

./scripts/fault_tree_generator.py -b 200 -a 5
scram --validate fault_tree.xml

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

scram-gui --help
scram-gui input/TwoTrain/two_train.xml&
sleep 5
kill $!

if [[ -z "${RELEASE}" && "$CXX" = "g++" ]]; then
  nosetests --with-coverage -w scripts test/
fi

LD_LIBRARY_PATH=${PWD}/install/lib/scram/test/:${LD_LIBRARY_PATH} \
  scram --validate --allow-extern install/share/scram/input/model/system_extern_library.xml
