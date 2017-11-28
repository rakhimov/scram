#!/usr/bin/env bash

set -ev

if [[ "${CXX}" = "icpc" ]]; then
  source ~/.bashrc
fi

ccache -s

install_cmd="time -p ./install.py --prefix=./install --threads 2"
if [[ -n "${RELEASE}" ]]; then
  ${install_cmd} --release
elif [[ "$CXX" = "g++" ]]; then
  ${install_cmd} --coverage --profile
else
  ${install_cmd} --debug
fi

ccache -s

./.travis/run_tests.sh

[[ -z "${RELEASE}" ]] || exit 0

if [[ "$CXX" = "clang++" ]]; then
  ./run-clang-format/run-clang-format.py -r gui/
  ./run-clang-format/run-clang-format.py -r src/
  ./run-clang-format/run-clang-format.py tests/*.{h,cc}
fi

[[ "$CXX" = "g++" ]] || exit 0
# Submit coverage of C++ and Python.
COV_DIR="build/src/CMakeFiles/scramcore.dir/"
TRACE_FILE="coverage.info"
lcov --no-compat-libtool --directory \
  $COV_DIR -c --rc lcov_branch_coverage=1 -o $TRACE_FILE -q 2> /dev/null
lcov --extract $TRACE_FILE '*/scram/*' -o $TRACE_FILE
codecov > /dev/null

# Check for memory leaks with Valgrind
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite \
  --errors-for-leak-kinds=definite --error-exitcode=127 \
  --track-fds=yes \
  scram_tests \
  --gtest_filter=-*Death*:*Baobab*:*IncorrectInclude*:*LabelsAndAttributes* \
  || [[ $? -ne 127 ]]

# Check documentation coverage
which doxygen || exit 1
doxygen ./.travis/doxygen.conf > /dev/null 2> doc_errors.txt
if [[ -s doc_errors.txt ]]; then
  echo "Documentation errors:" >&2
  cat doc_errors.txt >&2
  exit 1
fi

# Lizard function complexity printout for C++ and Python
lizard -w -L 60 -a 5 -EIgnoreAssert -ENS -Ecpre src gui \
  || echo "TODO: Fix the C++ complexity"
lizard -w -L 60 -a 5 scripts/*.py

# C++ linting
cpplint --repository=../ --quiet --recursive src/* 2> style.txt \
  || echo "TODO: Fix the C++ code"
cpplint --repository=../ --quiet --filter=-build/include_what_you_use \
  tests/* 2>> style.txt || echo "TODO: Fix the C++ code"

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
yapf -d ./*.py scripts/*.py scripts/test/*.py tests/*.py tests/input/fta/run_inputs.py
prospector ./*.py scripts/*.py
