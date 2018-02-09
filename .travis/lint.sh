#!/usr/bin/env bash

set -ev

CLANG_FORMAT="./run-clang-format/run-clang-format.py --clang-format-executable clang-format-${CLANG_VERSION}"

${CLANG_FORMAT} -r gui/
${CLANG_FORMAT} -r src/
${CLANG_FORMAT} tests/*.{h,cc}

# Check documentation coverage
which doxygen || exit 1
doxygen ./.travis/doxygen.conf > /dev/null 2> doc_errors.txt || (cat doc_errors.txt && exit 1)
doxygen ./.travis/gui_doxygen.conf > /dev/null 2>> doc_errors.txt || (cat doc_errors.txt && exit 1)
if [[ -s doc_errors.txt ]]; then
  echo "Documentation errors:" >&2
  cat doc_errors.txt >&2
  exit 1
fi
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
yapf -d scripts/*.py scripts/test/*.py tests/*.py tests/input/fta/run_inputs.py
prospector scripts/*.py

# TODO: Fix lupdate for C++17
# Qt lupdate warnings
# lupdate gui -ts gui/translations/scramgui_en.ts 2> ts_warnings.txt || (cat ts_warnings.txt && exit 1)
# if [[ -s ts_warnings.txt ]]; then
#   echo "Qt Lupdate warnings:" >&2
#   cat ts_warnings.txt >&2
#   exit 1
# fi
