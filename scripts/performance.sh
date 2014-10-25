#!/usr/bin/env bash

# This script runs SCRAM with analysis inputs that reveal common performance
# bottlenecks, such as probability calculation and minimal cut set calculation.
# This script uses input files and needs to be run in the 'scram/input'
# directory or in the directory where input files are located.
#
# NOTE: It is recommended to have SCRAM built in Release mode because the
#       tested cases in this script may take very long time.

echo -e '\n\n'
echo -e '200_Event -l 16'
scram 200_Event/fta_tree.xml -l 16 -r | head -n 15

echo -e '\n\n'
echo -e '200_Event -l 17'
scram 200_Event/fta_tree.xml -l 17 -r | head -n 15

echo -e '\n\n'
echo -e 'Baobab -l 6'
scram Baobab/baobab1.xml -l 6 | head -n 20

echo -e '\n\n'
echo -e 'Baobab -l 7'
scram Baobab/baobab1.xml -l 7 | head -n 20

echo -e '\n\n'
echo -e 'CEA9601 -l 4'
scram CEA9601/CEA9601.xml -l 4 | head -n 20

echo -e '\n\n'
echo -e 'CEA9601 -l 5'
scram CEA9601/CEA9601.xml -l 5 | head -n 20
