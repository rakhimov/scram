#####################################
SCRAM FTA: Common Cause Failure (CCF)
#####################################

#. Define groups of basic events that share common cause failure.

    - Groups must be unique.
    - Groups must not share members.
    - All members of a CCF group have the same description for failure.

#. Choose a model for analysis: MGL, alpha-factor, beta-factor, and phi-factor.

    - Factors must be provided by a user.
    - Calculation must be performed only with provided factors, or
      a user may specify the maximum number of common cause members
      in a group for analysis. Alternatively, for some types of
      CCF analysis methods, the maximum number of events for a set can be
      inferred by the number of provided factors.
    - There should be at least one and at most (N - 1) factors for
      MGL, alpha and phi-factor models. N is the number of members of the CCF
      group. For the beta-factor model, only one factor is required.
    - CCF grouping level numbers are optional, but it helps with input
      clarity and error-checking.
    - Levels of the provided factors are inferred positionally and
      sequentially. If a factor for any level is omitted, it is not implicitly
      assumed to be 0. The factors must be set to 0 explicitly. The exception
      is the factors that are above the last input level. In other words,
      only provided factors are used without requiring exactly (N - 1) factors.

#. After construction of the tree, calculate CCF sub groups for each event.

    - Validate groups.
    - Assign calculated probabilities to the newly created CCF sub-events.


#. Substitute CCF grouped primary events with OR gates
   with children as CCF-calculated sub groups. Give specific names to
   CCF sub groups that identify independent and common cause failures.

    - Events that are grouped by a common cause are listed in square brackets.

#. Perform usual analysis on the fault tree with CCF groups.

#. Report CCF specific information with common cause events grouped in
   square brackets.
