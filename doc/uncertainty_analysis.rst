####################
Uncertainty Analysis
####################

Pseudo-Random Number Generator
==============================

Uncertainty analysis employs Monte Carlo simulations
to calculate the uncertainty propagation in probabilities.
For Monte Carlo simulations,
SCRAM uses `MT 19937`_ Mersenne Twister Pseudo-random number generator.
Mersenne Twister PRNG is well tested and well suited for Monte Carlo simulations.

Given the same parameters for SCRAM simulations,
the same results are expected across different platforms and PRNG implementations
thanks to the standard specifications for MT 19937 in C++.
The seed of the PRNG is usually the system time at the runtime,
but this parameter can be fixed by a user,
for example, to test the analysis tool.

.. _MT 19937: https://en.wikipedia.org/wiki/Mersenne_twister


Monte Carlo (MC) Simulations
============================

#. Initialize events with distributions.
#. If uncertainty analysis is not requested,
   perform the standard analysis with average probabilities.
#. Set the seed for the PRNG for entire analysis. (Can be set by the user)
#. Determine the number of samples. (Can be set by the user)
#. Sample probability distributions and calculate the total probability.
#. Statistical analysis of the resulting distributions.
#. Sensitivity analysis. *Not Supported Yet*
#. Bayesian statistics. *Not Supported Yet*
#. Report the results of analysis:
   mean, sigma, quantiles, probability density histogram.


Statistical Distributions
-------------------------

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
