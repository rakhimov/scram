####################
Fault Tree Generator
####################

The complexity of a fault tree depends on many factors,
such as types of gates, the number of common nodes, the total number of nodes,
and the structure of the tree or the arrangement of the nodes.
It is best to craft fault trees to test complex and most demanding cases,
but it is time consuming to design large and non-trivial fault trees.
In order to facilitate the creation of complex fault trees,
a python script is provided that takes into account the factors
that make a fault tree complex for analysis
or other types of processing, such as visual representation and validation.


General Description
===================

- Use of pseudo-random numbers to determine the structure of the tree.
- The number of basic events is set by a user.
- The seed of the pseudo-random number generator is fixed and set by a user.
- Nodes in the fault tree are randomly chosen between a new node or common nodes.
- Names are assigned sequentially. B#, H#, CCF#, and G#.
- Probabilities for basic events are generated randomly.
- The fault tree is reproducible with the same parameters and the seed.
- The larger the fault tree is,
  the closer its characteristics are to the user-supplied parameters.
- The output is topologically sorted and valid for other analysis tools.


Script Arguments
================

- The seed for the pseudo-random number generator.
- The number of basic events.
- The number of house events.
- The number of CCF (MGL only) groups.
- The average number of arguments for gates.
- Percentage of common-basic-event arguments per gate.
  These events may show up in several places in the resulting fault tree.
- Percentage of common-gate arguments per gate.
  The acyclic property is ensured.
- The average number of parents for common basic events.
- The average number of parents for common gates.
- Minimum and maximum probabilities for basic events.
- Weights for the gate types: AND, OR, K/N, NOT, XOR.
  Complex gates (K/N, NOT, XOR) are created
  only if the weights are given.
- Output file name.
- Output formats: Aralia or XML(default).
- An option to merge gates into nested formulas for the output.

.. note::
    The number of gates can be constrained,
    but this may change other factors or not succeed at all.
    The reason is that the fault tree generation formulas
    may run out of degrees of freedom.


Note on Performance
===================

Depending on the provided arguments for the script,
the execution time varies greatly.
The number of gates and the number of common gates in the fault tree
tend to greatly increase the execution time
because of the need to prevent cycles.
If the number of gates and the number of common gates are kept constant,
the generation time scales linearly with the number of basic events.

It is possible to generate a 100,000-basic-event fault tree in less than a minute;
however, to generate more complex fault trees,
the Python code can be converted into C with Cython_,
which is capable of generating million-basic-event fault trees in few minutes.

.. _Cython:
    http://cython.org/
