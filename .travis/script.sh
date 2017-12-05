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
