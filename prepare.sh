#!/usr/bin/env bash

(cd src && ../lizard_report.sh > ../doc/complexity_report.txt)

doxygen doxygen.conf

make html

[[ -f doc/complexity_report.txt ]] && rm doc/complexity_report.txt
