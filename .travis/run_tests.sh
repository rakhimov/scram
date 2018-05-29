#!/usr/bin/env bash

set -ev

# Assuming that path contains the built binaries.
# And this script is called from the root directory.
which scram
which scram_tests
which scram-gui

scram_tests
(cd tests && python -m pytest test_scram_call.py)
(cd build && ctest --verbose)

./scripts/fault_tree_generator.py -b 200 -a 5 -o fault_tree.xml
scram --validate fault_tree.xml

[[ "${TRAVIS_OS_NAME}" == "linux" ]] || exit 0

scram-gui --help
scram-gui input/TwoTrain/two_train.xml&
sleep 5
kill $!

if [[ "${CONFIG}" == "Coverage" ]]; then
  (cd scripts && python -m pytest --cov=. --cov-config ../.coveragerc test/)
  mv scripts/.coverage ./
fi

LD_LIBRARY_PATH=${PWD}/build/lib/scram/:${LD_LIBRARY_PATH} \
  scram --validate --allow-extern tests/input/model/system_extern_library.xml
