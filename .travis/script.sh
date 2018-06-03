#!/usr/bin/env bash

set -ev

if [[ "${CXX}" = "icpc" ]]; then
  source ~/.bashrc
fi

ccache -s

cd build
install_cmd="cmake .. -DCMAKE_INSTALL_PREFIX=../install -DBUILD_TESTING=ON"
if [[ "${CXX}" == "g++" ]]; then
  install_cmd="${install_cmd} -DCMAKE_C_COMPILER=gcc-${GCC_VERSION} -DCMAKE_CXX_COMPILER=g++-${GCC_VERSION}"
fi

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
  TRACE_FILE="coverage.info"
  lcov --no-compat-libtool --directory build \
    --gcov-tool gcov-${GCC_VERSION} \
    -c -o $TRACE_FILE -q
  lcov --extract $TRACE_FILE --gcov-tool gcov-${GCC_VERSION} \
    '*/scram/*' -o $TRACE_FILE
  codecov > /dev/null
elif [[ "${CONFIG}" == "Memcheck" ]]; then
  # Check for memory leaks with Valgrind
  valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite \
    --errors-for-leak-kinds=definite --error-exitcode=127 \
    --track-fds=yes \
    scram_tests ~*Baobab* ~*IncorrectInclude* ~*LabelsAndAttributes* ~[.perf] \
    || [[ $? -eq 0 || $? -eq 1 ]]
fi
