#!/usr/bin/env bash

set -ev

if [[ "${CXX}" = "icpc" ]]; then
  source ~/.bashrc
fi

ccache -s

cd build
install_cmd="cmake .. -DCMAKE_INSTALL_PREFIX=../install"
if [[ -n "${RELEASE}" ]]; then
  ${install_cmd} -DCMAKE_BUILD_TYPE=Release
elif [[ "$CXX" = "g++" ]]; then
  ${install_cmd} -Wdev -DCMAKE_BUILD_TYPE=Debug -DWITH_PROFILE=ON -DWITH_COVERAGE=ON
else
  ${install_cmd} -DCMAKE_BUILD_TYPE=Debug
fi

time -p make install -j 2
cd ..

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
