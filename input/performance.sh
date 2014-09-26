#!/usr/bin/env bash

echo -e 'Three Motor -c 1e-8'
scram ThreeMotor/three_motor.xml -c 1e-8 | grep 'Probability'

echo -e '\n\n'
echo -e 'Three Motor -c 0'
scram ThreeMotor/three_motor.xml -c 0 | grep 'Probability'

echo -e '\n\n'
echo -e '200_Event -l 16'
scram 200_Event/fta_tree.xml -l 16 | head -n 15

echo -e '\n\n'
echo -e '200_Event -l 17'
scram 200_Event/fta_tree.xml -l 17 | head -n 15

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
