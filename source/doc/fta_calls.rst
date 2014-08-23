#########################################
SCRAM Fault Tree Analysis Functionality
#########################################

Current Command-line Functionality:
====================================
#. Check and validate an input file with a fault tree description.
#. Check a probability file together with an input tree file.
#. Validate XML input files against RelaxNG. [not implemented]
#. Validate the input analysis specifications and instantiations. [not implemented]
#. Output a graphing dot file. *No probability definition file needed*
#. Find minimal cut sets. *No probability definition file needed*

   - May specify maximum order for cut sets for faster calculations.
   - May specify cutoff probability for cut sets. [not implemented]

#. Find the total probability of the top event.

   - May specify the rare event approximation or MCUB for faster calculations.
