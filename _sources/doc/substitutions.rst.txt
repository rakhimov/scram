#############
Substitutions
#############

With the notion of Substitutions,
the Model Exchange Format [MEF]_ provides a general way to express
Delete Terms, Recovery Rules, and Exchange Events.
As noted in the MEF specification,
*non-declarative* substitutions (e.g., Exchange Events) should be avoided
or replaced with event-tree instructions if possible.
Moreover, the *non-declarative* substitutions come with extra performance penalty
since this approach forces set manipulations at post-analysis
and may require analysis re-evaluation
(e.g., the application of truncations at analysis time may become corrupted).

.. note:: Non-declarative substitutions are applied only to minimal cut sets
          (i.e., no exact-probability BDD or prime implicants).


Validation
==========

- Hypothesis and source events must be unique (no duplicates allowed).
- Hypothesis formulas must be built-over basic events only (e.g., no gates or nested formulas).
- Hypothesis formulas must be coherent.
- If a substitution is non-declarative (i.e., the source is not empty),
  its hypothesis can only be defined with OR/AND/NULL connectives.
- The optional "traditional" type is helpful and declared for validation purposes only.
  The validity error is detected if the declared type does not match the deduced one.
- If a declarative substitution (i.e., empty source) has constant target ``true``,
  the substitution has no effect and is considered invalid.
- If a non-declarative substitution has constant target ``false``,
  the substitution is malformed since the source set is irrelevant
  (it should have been declarative delete-terms).
- Non-declarative substitution hypothesis, source, and target events cannot be in CCF groups.
- Since the order of *non-declarative* substitutions is unspecified,
  the application of *all* substitutions must be idempotent regardless of their order
  (i.e., the composition of substitutions must be commutative).
  The following requirements apply only to *non-declarative* substitutions:

    #. No target event can be a source event of any substitution.
    #. No target or source event can be an argument of another substitution hypothesis.
