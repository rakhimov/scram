#########################################
SCRAM Fault Tree Analysis Functionality
#########################################

.. note:: Run '*scram -h*' from command line to get more descriptions.

Current Command-line Functionality:
====================================
#. Validate XML input files against RelaxNG.
#. Validate the input analysis specifications and instantiations.
#. Output a graphing dot file. *No probability definition required*
#. Find minimal cut sets. *No probability definition required*

   - May specify maximum order for cut sets for faster calculations.
   - May specify cut-off probability for cut sets.

#. Find the total probability of the top event.

   - May specify the rare event approximation or MCUB for faster calculations.
