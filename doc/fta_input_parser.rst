#################################################
SCRAM FTA: Input Parser
#################################################

Steps in Fault Tree Input Verification
========================================
#. Ignore comment lines.
#. Find an opening brace '{'.
#. Check input values.
#. Each line contains only two strings.
#. Find closing brace '}'.
#. Create a node:

    - Parent exists and is not a primary event (*may change for CCF*).
    - Non-primary event has a unique name in an input file or a sub-tree.
    - Primary events with several parents are allowed.
    - Each gate has a correct number of children.
    - Store TransferIn gate for later inclusion.
    - Include trees from Transfer files.

#. Throw an error with a message (a file name, line numbers, types of errors).
#. Check probabilities for primary events:

    - Report missing probabilities.
    - Throw an error if an event is defined doubly.
    - Ignore events that are not initialized in the tree when assinging
      probabilities from a probability file.
