#!/usr/bin/env bash

# Assuming that path contains the built binaries.
# And this script is called from the root directory.
scram_unit_tests
cd ./tests
nosetests
cd ..
