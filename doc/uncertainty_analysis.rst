####################
Uncertainty Analysis
####################

Uncertainty analysis employs Monte Carlo simulations to calculate the
uncertainty propagation in probabilities.
For Monte Carlo simulations, `MT 19937`_ Mersenne Twister Pseudo-random number
generator is provided by Boost_. Mersenne Twister PRNG is well tested and
well suited for Monte Carlo simulations. The seed of the PRNG is usually
the system time at the runtime, but this parameter can be fixed by a user,
for example, to test the analysis tool.

.. _`MT 19937`:
    https://en.wikipedia.org/wiki/Mersenne_twister
.. _Boost:
    http://www.boost.org/doc/libs/1_56_0/doc/html/boost_random/reference.html


Monte Carlo (MC) Simulation Implementation
==========================================

#. Initialization of events with distributions.
#. If uncertainty analysis is not requested, perform the standard analysis with
   average probabilities.
#. Generation of an equation for MC simulations from minimal cut sets.
#. Set the seed for the PRNG for entire analysis. (Can be set by a user)
#. Determine the number of samples. (Can be set by a user)
#. Definitions and parameters for various distributions:

    - May be specified by a user in an input file.
    - Distributions:

        * Uniform
        * Triangular
        * Piecewise Linear
        * Histogram
        * Discrete
        * Normal
        * Log-Normal
        * Poisson
        * Binomial
        * Beta
        * Gamma
        * Weibull
        * Exponential
        * Log-Uniform
        * Log-Triangular

#. Sampling probability distributions and calculating the total equation.
#. Statistical analysis of the resulting distributions.
#. Sensitivity analysis. *Not Supported Yet*
#. Bayesian statistics. *Not Supported Yet*
#. Output the results of analysis: mean, sigma, quantiles,
   probability density histogram.
