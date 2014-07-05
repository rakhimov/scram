#!/usr/bin/env bash

# Assuming that path contains the built binaries.
# And this script is called from the root directory.
if which scram_unit_tests && which scram
then
  if scram_unit_tests
  then
    if nosetests -w ./tests/
    then
      # Check generation and graphing of a complex tree
      if ./input/tree_generator.py -p 200 -c 5
      then
        if scram -g fta_tree.scramf
        then
          dot -Tsvg fta_tree.dot -o fta_tree.svg
        fi
      fi
    fi
  fi
fi

exit
