#!/usr/bin/env bash

set -ev
set -o pipefail

CLANG_FORMAT="python2 ./run-clang-format/run-clang-format.py --clang-format-executable clang-format-${CLANG_VERSION}"

${CLANG_FORMAT} -r gui/
${CLANG_FORMAT} -r src/
${CLANG_FORMAT} tests/*.{h,cc}

# Check documentation coverage
doxygen ./.travis/doxygen.conf
doxygen ./.travis/gui_doxygen.conf

ls src/**/*.{cc,h} | xargs -n 1 grep -q '^/// @file$'  # Missing file doc.
ls gui/*.{cpp,h} | xargs -n 1 grep -q '^/// @file$'  # Missing file doc.

# Lizard function complexity printout for C++ and Python
lizard -w -L 60 -a 5 -EIgnoreAssert -ENS -Ecpre src gui \
  || echo "TODO: Fix the C++ complexity"
lizard -w -L 60 -a 5 scripts/*.py

# C++ linting
cpplint --quiet --recursive src/
cpplint --quiet tests/*.{cc,h,cc.in}

# Python linting
yapf -d scripts/*.py scripts/test/*.py tests/*.py
prospector scripts/*.py
