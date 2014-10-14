#!/usr/bin/env bash

# Series of commands to profile SCRAM using Gprof.
# This script must be run in the root scram directory.

rm -rf build 2> /dev/null
mkdir build 2> /dev/null
cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=-pg -DCMAKE_CXX_FLAGS=-pg -DCMAKE_INSTALL_PREFIX=../../install/ && \
  make -j 2 install
if [ $? -ne 0 ]; then exit 1; fi
cd ../input && \
  scram 200_Event/fta_tree.xml -l 16 > /dev/null && \
  gprof ../build/bin/scram > 200_prof.txt
cd ..
rm -rf build 2> /dev/null
mkdir build 2> /dev/null
cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=-pg -DCMAKE_CXX_FLAGS=-pg -DCMAKE_INSTALL_PREFIX=../../install/ && \
  make -j 2 install
if [ $? -ne 0 ]; then exit 1; fi
cd ../input && \
  scram ThreeMotor/three_motor.xml > /dev/null && \
  gprof ../build/bin/scram > three_motor_prof.txt
