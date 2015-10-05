####################
Uncertainty Analysis
####################

Pseudo-Random Number Generator
==============================

Uncertainty analysis employs Monte Carlo simulations
to calculate the uncertainty propagation in probabilities [UA]_.
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


Adjustment of Invalid Samples
-----------------------------

Upon performing Monte-Carlo simulations,
the analysis is not required
to check the validity of samples
and to abort the analysis.
If it happens
that a sampled value is not sensible (probability > 1),
the sample is adjusted to the nearest acceptable value (probability = 1).
No warnings are given in this case;
however, if the problem is due to probability calculation approximations (rare-event),
Probability Analysis report may contain the warning.

In order to prevent invalid ranges for uncertainty analysis,
the input validation assesses the ranges of provided distributions.
Uncertainty analysis is performed
only if maximum and minimum values are within the acceptable range.
This approach should prevent most errors.

However, for some distributions (normal),
theoretical range may be (-inf, +inf),
which rarely fits for probability analysis variables.
In this case,
the best estimate is made to find the most likely range
(5-6 sigma, 99.9% percentile, etc.).
