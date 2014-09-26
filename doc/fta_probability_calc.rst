.. _prob_calc:

###############################################
SCRAM FTA: Probability Calculations
###############################################

Since minimal cut sets may neither be mutually exclusive
nor independent, direct use of the sets' total probabilities may be inaccurate.
In addition, rare event approximation may not be appropriate
for events with large probabilities.

In order to account for the above deficiencies, brute force algorithm is
implemented to calculate total probability of a top event in a
fault tree. However, note that this algorithm still assumes independence of
primary events in the fault tree.


The Exact Probability Calculation Implementation
========================================================

The general probability formula for sets is expanded recursively.
In each iteration, some sets are merged in order to account for common members
in minimal cut sets. This algorithm is also used for series expansion, giving
the Sylvester-Poincaré expansion.

The Approximate Probability Calculation Implementation
========================================================

Approximate calculations are implemented in order to reduce calculation
time. The series expansion of the exact formula is applied.
This feature is the default. The default value for series is set to N
(*number of the sets*) to get all the series included.
In general, it is impractical to include more than 8 sums, so the suggestion
is to include between 4 and 8 sums.
In addition, cut-off probability for cut sets can be applied to discard
sets with low probabilities.

The Rare-Event Approximation
=============================
Given that probabilities of events are very small value less than 0.1, only the
first series in the probability equation may be used as a conservative
approximation; that is, the total probability is the sum of all probabilities
of minimal cut sets. Ideally, this approximation gives good results for
independent minimal cut sets with very low probabilities. However, if the cut
set probabilities are high, the total probability may exceed 1.0.

The Min-Cut-Upper Bound (MCUB) Approximation
=============================================
This method calculates the total probability by subtracting the probability
of all minimal cut sets being successful from 1.0; thus, the total probability
never exceeds 1.0. Non-independence of the minimal cut sets introduce
the major discrepancy for this technique. However, the MCUB approximation
provides non-conservative estimation for non-coherent trees containing
*NOT* logic. There are other limitations described by Don Wakefield in
"You Can't Just Build Trees and Call It PSA."

Probability Types
========================================================

**P-model**
    The probability of an event occurring when the time to failure is
    unknown or unpredictable.

**Lambda-model**
    The probability that a primary event will occur within
    a given period of time (t). Appropriate for events within
    systems that are continuously operating and have a known
    probability of failure during a unit time period (:math:`\lambda`).
    The event probability is equal to

    .. math::

        P = 1-\exp(-\lambda*t)

    For small lambda and time, the approximation is

    .. math::

        P \approx \lambda*t
