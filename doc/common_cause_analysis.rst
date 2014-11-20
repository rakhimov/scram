#####################################
SCRAM FTA: Common Cause Failure (CCF)
#####################################

#. Define groups of basic events that share common cause failure.

    - Groups must not share members.
    - All members of a CCF group have the same description for failure.

#. Choose a model for analysis: MGL, alpha-factor, beta-factor, and phi-factor.

    - Factors must be provided by a user.
    - Calculation must be performed only with provided factors, or
      a user may specify the maximum number of common cause members
      in a group for analysis. Alternatively, for some types of
      CCF analysis methods, the maximum number of events for a set can be
      inferred by the number of provided factors.

#. After construction of the tree, calculate CCF sub groups for each event.

#. Convert CCF grouped primary events into intermediate events with
   OR logic with children as CCF sub groups. Give specific names to
   CCF sub groups that identify independent and common cause failures.
   This operation will change the CCF-grouped primary events to OR gates.

#. Assign calculated probabilities to the newly created CCF sub-events.

#. Perform usual analysis on the updated fault tree.

#. Report CCF specific information.
