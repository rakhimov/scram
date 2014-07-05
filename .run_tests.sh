#!/usr/bin/env bash

# Assuming that path contains the built binaries.
# And this script is called from the root directory.
if which scram_unit_tests && which scram
then
  if scram_unit_tests
  then
    nosetests -w ./tests/
  fi
fi

exit
