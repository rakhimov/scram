#################
SCRAM Description
#################

This is a general description of SCRAM and possible future additions.
These descriptions may not reflect current capabilities.

Command-line Call of SCRAM by a User
=====================================

Type of analysis, i.e., fault tree, even tree, Markov chain, etc.
Upon calling from command-line, the user should indicate which analysis
should be performed. In addition to this, the user may provide
additional parameters for the analysis type chosen. For example, if
FTA is requested, type of algorithm for minimal cut set generation may
be specified by the user, or the user can choose if probability
calculations should assume the rare event or MCUB approximation.
However, there are default values for these parameters that try to
give the most optimal and accurate results.


Fault Tree Analysis
====================

#. Reading input files. Verification of the input. Optional visual tool.

#. (*Optional*) Upon users' request output instruction file for graphing
   the tree. This is for visual verification of the input.
   Stop execution of the program.

#. Instantiate the analysis. Create the trees for the analysis.

#. Perform cut set generation or other analysis.

#. Perform numerical analysis:
   Rare event approximation and independence of events.
   Rare event approximation is used only if enforced by a user.

#. Output the user specified analysis results. The output is sorted by
   the order of minimal cut sets, their probabilities. In addition,
   the contribution of each primary event is given in the output.


Event Tree Analysis
====================
#. Reading input files. Verification of the input. Optional visual tool.
#. (*Optional*) Upon users' request output instruction file for **graphviz**
   dot to draw the tree. This is for visual verification of the input.
   Stop execution of the program.
#. Create the tree for analysis.
#. Perform calculations.
#. Output the results.


Future Additions
=================
#. Event tree analysis.
#. Various other algorithms for fault tree analysis.
#. Dynamic fault tree analysis.
#. Monte Carlo Methods.
#. Markov analysis.


General Information for Users
==============================
#. Run 'scram -h' to see all the flags and parameters for analysis.

#. The minimum cut set generation for a fault tree and probability calculations
   may use a lot of time and computing power depending on the complexity of
   the tree. You can adjust SCRAM flags and parameters to reduce these demands.
