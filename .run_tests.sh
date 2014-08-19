#!/usr/bin/env bash

# Assuming that path contains the built binaries.
# And this script is called from the root directory.
which scram_unit_tests && which scram \
  && scram_unit_tests \
  && nosetests -w ./tests/ \
  && ./input/tree_generator.py -p 200 -c 5 \
  && scram -g fta_tree.scramf \
  && dot -Tsvg fta_tree.dot -o fta_tree.svg

exit $?
