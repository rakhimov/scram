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
cpplint --repository=../ --quiet --recursive src/* 2> style.txt \
  || echo "TODO: Fix the C++ code"

# Clean false positives and noise
sed -i '/Found C system header after C\+\+/d' style.txt
sed -i '/Found C system header after other header/d' style.txt
sed -i '/Include the directory when naming/d' style.txt
sed -i '/^Ignoring/d' style.txt
sed -i '/^Skipping/d' style.txt
sed -i '/is an unapproved C++11 header/d' style.txt
sed -i '/Add #include <algorithm> for sort/d' style.txt
if [[ -s style.txt ]]; then
  echo "Style errors:" >&2
  cat style.txt >&2
  exit 1
fi

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
