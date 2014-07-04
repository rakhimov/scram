#!/usr/bin/env bash
#
# Run tests for Coveralls.

cd build
mkdir report
cd ./src/CMakeFiles/scram.dir/
coveralls --dryrun --exclude lib --exclude tests --exclude CMakeFiles --exclude build > /dev/null
cp ./*.cc.gcov ../../../report/
cd ../scramcore.dir/
coveralls --dryrun --exclude lib --exclude tests --exclude CMakeFiles --exclude build > /dev/null
cp ./*.cc.gcov ../../../report/

for f in error.h.gcov event.h.gcov fault_tree.h.gcov risk_analysis.h.gcov superset.h.gcov
do
  if [ -f f ]; then
    cp f ../../../report/
  fi
done

cd ../../../report/
coveralls --no-gcov --exclude lib --exclude tests --exclude CMakeFiles --exclude build
