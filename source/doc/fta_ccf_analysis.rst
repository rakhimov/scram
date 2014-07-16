########################################
SCRAM FTA: Common Cause Failure (CCF)
########################################

.. note:: This functionality is not yet present.

#. Define groups of basic events that share common cause failure.

    - Can groups share memebers?

#. Choose a model for analysis: MGL, alpha, beta, etc.

    - Factors must be provided by a user
    - Calculation must be performed with only with provided factors, or
      a user may specify the maximum number of common cause members
      can be in a group for analysis. Alternatively, for some types of
      CCF analysis methods, the maximum number of events for a set can be
      inferred by the number of provided factors.

#. After constructing the main tree, calculate ccf sub groups for each event.

#. Then, convert ccf grouped primary events into intermediate events with
   OR logic with children as ccf sub groups. Give specific names to
   ccf sub groups that identify independent and common cause failures.
   This will require removing the primary event from primary events database
   and creating a respective intermediate event with ccf groups.
   May consider re-assigning pointers, or getting parents of the basic event
   and re-assigning their children to the newly created intermediate event.

#. Assigning probabilities should be handled in a similar way, replacing
   basic event probability with calculated probabilities for sub-sets.

#. Perform usual analysis on the updated tree.

#. Output formatting may change to indicate CCF.
