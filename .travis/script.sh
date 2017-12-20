#!/usr/bin/env bash

set -ev

if [[ "${CXX}" = "icpc" ]]; then
  source ~/.bashrc
fi

ccache -s

cd build
install_cmd="cmake .. -DCMAKE_INSTALL_PREFIX=../install"
if [[ "${CONFIG}" == "Coverage" ]]; then
  ${install_cmd} -Wdev -DCMAKE_BUILD_TYPE=Debug -DWITH_COVERAGE=ON
elif [[ "${CONFIG}" == "Memcheck" ]]; then
  ${install_cmd} -DCMAKE_CXX_FLAGS="-g -Og" -DBUILD_GUI=OFF
else
  ${install_cmd} -DCMAKE_BUILD_TYPE="${CONFIG}"
fi

time -p make install -j 2
cd ..

ccache -s

if [[ "${CONFIG}" != "Memcheck" ]]; then
  ./.travis/run_tests.sh
fi

if [[ "${CONFIG}" == "Coverage" ]]; then
  # Submit coverage of C++ and Python.
  COV_DIR="build/src/CMakeFiles/scram.dir/"
  CLI_COV_DIR="build/src/CMakeFiles/scram-cli.dir/"
  GUI_COV_DIR="build/gui/CMakeFiles/scram-gui.dir/"
  TRACE_FILE="coverage.info"
  lcov --no-compat-libtool --directory $COV_DIR --directory $CLI_COV_DIR \
    --directory $GUI_COV_DIR \
    -c --rc lcov_branch_coverage=1 -o $TRACE_FILE -q
  lcov --extract $TRACE_FILE '*/scram/*' -o $TRACE_FILE
  codecov > /dev/null
elif [[ "${CONFIG}" == "Memcheck" ]]; then
  # Check for memory leaks with Valgrind
  valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite \
    --errors-for-leak-kinds=definite --error-exitcode=127 \
    --track-fds=yes \
    scram_tests \
    --gtest_filter=-*Death*:*Baobab*:*IncorrectInclude*:*LabelsAndAttributes* \
    || [[ $? -eq 0 || $? -eq 1 ]]
fi
