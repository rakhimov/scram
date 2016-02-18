#!/usr/bin/env bash

make html

(cd src && ../lizard_report.sh > ../doc/complexity_report.txt)

doxygen doxygen.conf

[[ -f doc/complexity_report.txt ]] && rm doc/complexity_report.txt

cp -r ./publish ./build/
