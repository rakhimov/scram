#########################################
SCRAM FTA: Transfer Tree
#########################################

Specifics of Transfer In and Transfer Out
================================================
- Root TransferIn may be used to analyze a specific sub-tree.

- Get the list of transfers.

    * If transfer is a root, then check if there are other nodes in the main
      file. If so, it is a mistake. (Checked implicitly.)
    * Transfer can list their parent as only an intermediate event or top
      event. No dangling transfers. (Checked just like other nodes.)
    * Intermediate event can have many TransferIn.
    * Only **one** TransferOut per intermediate event.
    * TransferOut cannot be a primary event.

- Check if the transfer list is initiated correctly.

    * Error messages should indicate the name of the file from which the
      sub-tree is being transported.
    * Only one intermediate event can follow the transfer out.
    * None of the elements in the transfer tree should refer to elements in
      the original tree. This is eliminated by appending subtree name to every
      intermediate event for internal calculations. However, this should not
      show up for graphing.
    * Transfer Intermediate events may show up in several places and cause
      conflicts with the uniqueness of intermediate events.
      May append some suffix to avoid problems, such as number of
      references. *Not Yet Implemented The Right Way*

- Update database of top, inter, and primary events after reading the
  sub-tree. *Currently is not done in isolation*

- Keep track of cyclic inclusions and prevent it with an error.

.. note::
    - If a user wants to re-use an intermediate event in multiple places,
      transfers should be used instead of finding unique names for those
      intermediate events with the same structure and primary events.
