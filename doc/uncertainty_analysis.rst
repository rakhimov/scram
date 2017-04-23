.. _uncertainty_analysis:

####################
Uncertainty Analysis
####################

Pseudo-Random Number Generator
==============================

Uncertainty analysis employs Monte Carlo simulations
to calculate the uncertainty propagation in probabilities [UA]_.
For Monte Carlo simulations,
SCRAM uses `MT 19937`_ Mersenne Twister Pseudo-random number generator
provided by the C++ standard library.
Mersenne Twister PRNG is considered well tested and well suited for Monte Carlo simulations.

Given the same parameters for SCRAM simulations,
the same results are expected across runs on the same platforms.
However, the same results across platforms and library implementations are not guaranteed.
Even though the PRNG produces the same stable sequence
across different platforms and PRNG implementations
thanks to the standard specifications for MT 19937 in C++,
the implementation of statistical distributions is library specific
and not guaranteed to produce the same results across platforms.

The default seed of the PRNG is 5489 (C++ specification),
but this parameter can be changed by a user,
for example, to test the analysis tool.

.. _MT 19937: https://en.wikipedia.org/wiki/Mersenne_twister


Monte Carlo (MC) Simulations
============================

#. Initialize events with distributions.
#. If uncertainty analysis is not requested,
   perform the standard analysis with mean probabilities.
#. Set the seed for the PRNG for entire analysis. (Can be set by the user)
#. Determine the number of samples/trials. (Can be set by the user)
#. Sample probability distributions and calculate the total probability.
#. Statistical analysis of the resulting distributions.
#. Sensitivity analysis. *Not Supported Yet*
#. Report the results of analysis:
   mean, sigma, quantiles, probability density histogram.


Statistical Distributions
-------------------------

- Uniform
- Triangular (only via API)
- Piecewise Linear (only via API)
- Histogram
- Discrete (only via API)
- Normal
- Log-Normal
- Poisson (only via API)
- Binomial (only via API)
- Beta
- Gamma
- Weibull
- Exponential (only via API)
- Log-Uniform (only via API)
- Log-Triangular (only via API)


Adjustment of Invalid Samples
-----------------------------

Upon performing Monte-Carlo simulations,
the analysis is not required
to check the validity of samples
and to abort the analysis.
If it happens
that a sampled value is not sensible (e.g., probability > 1),
the sample is adjusted to the nearest acceptable value (e.g., probability = 1).
No warnings are given in this case;
however, if the problem is due to probability calculation approximations (the rare-event),
Probability Analysis report may contain the warning message.

In order to prevent invalid ranges for uncertainty analysis,
the input validation assesses the ranges or sampling intervals of provided distributions.
Uncertainty analysis is performed
only if the intervals are within acceptable ranges (e.g., [0, 1] probability interval).
This approach should prevent most errors.

However, for some distributions (e.g., normal),
theoretical range may be unbounded (e.g., (-inf, +inf)),
which rarely fits probability analysis variables.
In this case of unbounded intervals,
the best estimate is made to find the most likely range or real-valued bounds
(5-6 sigma, 99.9% percentile, etc.).
