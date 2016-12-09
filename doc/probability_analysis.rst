.. _prob_calc:

####################
Probability Analysis
####################

*****************
Probability Types
*****************

Various probability types and distributions are accepted
as described in the Open-PSA Model Exchange Format [MEF]_,
for example, constant values, exponential with two or four parameters,
and uniform, normal, log-normal distributions.

Bellow is a brief description.
For more information, please take a look at the [MEF]_ format documentation.

P-model
    The probability of an event occurring
    when the time to failure is unknown or unpredictable.

Lambda-model or Exponential with Two Parameters
    The probability that a primary event will occur within given time (:math:`t`).
    Appropriate for events within systems
    that are continuously operating and
    have a known probability of failure during a unit time period (:math:`\lambda`).

    .. math::

        P = 1-e^{-\lambda \cdot t}

    For small lambda and time, the approximation is

    .. math::

        P \approx \lambda \cdot t


************************
Probability Calculations
************************

Probability calculation algorithms assume
independence of basic events in the fault tree.
The dependence can be communicated with common cause groups.


The Exact Probability Calculation
=================================

Since the resultant sets may neither be mutually exclusive nor independent,
direct use of the sets' total probabilities may be inaccurate [WakXX]_.
The exact probability calculation is achieved
with Binary Decision Diagram ([BDD]_) based algorithms [DR01]_.
This approach does not require calculation of products.
As long as a fault tree ([PDAG]_) can be converted into BDD,
the calculation of its probability is linear in the size of BDD.


The Approximate Probability Calculation
=======================================

Approximate calculations are implemented to reduce the calculation time.
However, the users must be aware of the limitations and inaccuracies of approximations [WakXX]_.
If approximate calculations yield probability values above one,
the result is adjusted to one.
In this special case,
appropriate warnings are given in the final report.


The Rare-Event Approximation
----------------------------

Given that the probabilities of events are very small value less than 0.1,
only the first series in the probability equation may be used
as a conservative approximation;
that is, the total probability is the sum of all probabilities of minimal cut sets.
Ideally, this approximation gives good results
for independent minimal cut sets with very low probabilities.
However, if the cut set probabilities are high,
the total probability may exceed 1.

This is the default quantitative approximation
for analysis algorithms that inherently run with approximations (MOCUS and ZBDD).


The Min-Cut-Upper Bound (MCUB) Approximation
--------------------------------------------

This method calculates the total probability
by subtracting the probability of all minimal cut sets' being successful from 1;
thus, the total probability never exceeds 1.
Non-independence of the minimal cut sets introduce the major discrepancy for this technique.
Moreover, the MCUB approximation provides non-conservative estimation
for non-coherent trees containing NOT logic [WakXX]_.


*******************
Importance Analysis
*******************

Importance analysis is performed for basic events in a fault tree [DR01]_.
The same configurations are used as for probability analysis.
The analysis is performed by request with probability data.
The following importance factors are calculated:

    - Fussel-Vesely Diagnosis Importance Factor (DIF)
    - Birnbaum Marginal Importance Factor (MIF)
    - Critical Importance Factor (CIF)
    - Risk Reduction Worth (RRW)
    - Risk Achievement Worth (RAW)

The short description and interpretation of the factors
can be found in :ref:`theory`.
