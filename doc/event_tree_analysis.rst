.. _event_tree_analysis:

###################
Event Tree Analysis
###################

Event tree analysis [ETA]_ is an analysis of end sequences
given an initiating event and progression of states over functional events.
The analysis uses the gathered Boolean formula and expressions per sequence
to supply arguments for other analysis kinds,
such as fault-tree, probability, and uncertainty analyses.

The Model Exchange Format [MEF]_ provides very flexible and powerful mechanisms
to express event trees.
The programming nature of event trees is achieved with a set of Instructions and Expressions
that control the interpretation of paths leading to sequences.

Event tree validation notes:

    - The sequences defined in event trees are public (i.e., global).
      In other words, two different event trees cannot define sequences with the same name,
      but either event tree can reference each others' sequences.

    - The order of functional events is deduced implicitly
      from the declaration of the functional events in the event tree.
      This order must be respected in event tree paths.

    - Functional event states in forks can be arbitrary but unique (per fork).

    - In expressions evaluating a functional event state,
      the state must be one of possible states declared in functional event forks.
      *This validation is not implemented.*

    - The event-tree is exclusively defined by either formulas or expressions,
      but no mix is allowed.

    - In general (fault-tree linking, event-tree linking),
      the validation of mutual-exclusivity, completeness (sum to 1), or conditional-independence
      is not performed.
