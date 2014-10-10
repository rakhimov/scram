######################################
SCRAM Monte Carlo Uncertainty Analysis
######################################

For Monte Carlo simulations, `MT 19937`_ Mersenne Twister Pseudo-random number
generator is provided by Boost_. Mersenne Twister PRNG is well tested and
well suited for Monte Carlo simulations. The seed of the PRNG is usually
the system time at the runtime, but this parameter can be fixed by a user,
for example, to test the analysis.

.. _`MT 19937`:
    https://en.wikipedia.org/wiki/Mersenne_twister
.. _Boost:
    http://www.boost.org/doc/libs/1_56_0/doc/html/boost_random/reference.html

Monte Carlo (MC) Analysis Implementation
========================================

#. Initializing events with distributions. *Not Supported*

#. If MC Analysis is not requested, perform the standard analysis with
   average probabilities. *Not Supported*

#. Generation of an equation for MC analysis from minimal cut sets.

#. Determine the number of samples. (Set by a user) *No Support*

#. Definitions and parameters for various distributions. *Not Supported Yet*

    * May be specified by a user in an input file.
    * Distributions:

        - Uniform
        - Triangular
        - Piecewise Linear
        - Histogram
        - Discrete
        - Normal
        - Log-Normal
        - Poisson
        - Binomial
        - Beta
        - Gamma
        - Weibull
        - Exponential
        - Log-Uniform
        - Log-Triangular

#. Sampling probability distributions and calculating the total equation.
   *Not Supported Yet*

#. Statistical analysis. The resulting distributions. *Not Supported Yet*
#. Sensitivity analysis. *Not Supported Yet*
#. Bayesian statistics. *Not Supported Yet*
#. Output the results of analysis: mean, sigma, probability density plot. *No*
