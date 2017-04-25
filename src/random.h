/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file random.h
/// Contains helpers for randomness simulations.

#ifndef SCRAM_SRC_RANDOM_H_
#define SCRAM_SRC_RANDOM_H_

#include <cassert>
#include <cmath>

#include <random>

#include <boost/random/beta_distribution.hpp>
#include <boost/random/triangle_distribution.hpp>

namespace scram {

/// This class contains generators for various random distributions.
/// The values passed to the member functions are asserted
/// to be in the correct form.
/// In other words, the user should make sure
/// that the passed parameters are valid.
/// For example, standard deviation cannot be negative.
///
/// This facility wraps the engine and distributions.
/// It provides convenience and reproducibility for the whole analysis.
class Random {
 public:
  /// Sets the seed of the underlying random number generator.
  ///
  /// @param[in] seed  The seed for RNGs.
  static void seed(int seed) noexcept {
    Random::rng_.seed(static_cast<unsigned>(seed));
  }

  /// RNG from a uniform distribution.
  ///
  /// @param[in] lower  Lower bound.
  /// @param[in] upper  Upper bound.
  ///
  /// @returns A sampled value.
  static double UniformRealGenerator(double lower, double upper) noexcept {
    assert(lower < upper);
    return std::uniform_real_distribution<>(lower, upper)(rng_);
  }

  /// RNG from a triangular distribution.
  ///
  /// @param[in] lower  Lower bound.
  /// @param[in] mode  The peak of the distribution.
  /// @param[in] upper  Upper bound.
  ///
  /// @returns A sampled value.
  static double TriangularGenerator(double lower, double mode,
                                    double upper) noexcept {
    assert(lower < mode);
    assert(mode < upper);
    return boost::random::triangle_distribution<>(lower, mode, upper)(rng_);
  }

  /// RNG from a piecewise linear distribution.
  ///
  /// @tparam IteratorB  Input iterator of interval boundaries returning double.
  /// @tparam IteratorW  Input iterator of weights returning double.
  ///
  /// @param[in] first_b  The begin of the interval boundaries.
  /// @param[in] last_b  The sentinel end of the interval boundaries.
  /// @param[in] first_w  The begin of the interval weights.
  ///
  /// @returns A sampled value.
  ///
  /// @pre Interval points for the distribution must be strictly increasing.
  ///
  /// @pre The number of weights must be equal to
  ///      the number of boundaries.
  ///      Extra weights are ignored.
  template <class IteratorB, class IteratorW>
  static double PiecewiseLinearGenerator(IteratorB first_b, IteratorB last_b,
                                         IteratorW first_w) noexcept {
    return std::piecewise_linear_distribution<>(first_b, last_b, first_w)(rng_);
  }

  /// RNG from a histogram distribution.
  ///
  /// @tparam IteratorB  Input iterator of interval boundaries returning double.
  /// @tparam IteratorW  Input iterator of weights returning double.
  ///
  /// @param[in] first_b  The begin of the interval boundaries.
  /// @param[in] last_b  The sentinel end of the interval boundaries.
  /// @param[in] first_w  The begin of the interval weights.
  ///
  /// @returns A sampled value from the interval.
  ///
  /// @pre Interval points for the distribution must be strictly increasing.
  ///
  /// @pre The number of weights must be equal to
  ///      the number of intervals (boundaries - 1).
  ///      Extra weights are ignored.
  template <class IteratorB, class IteratorW>
  static double HistogramGenerator(IteratorB first_b, IteratorB last_b,
                                   IteratorW first_w) noexcept {
    std::piecewise_constant_distribution<> dist(first_b, last_b, first_w);
    return dist(rng_);
  }

  /// RNG from a discrete distribution.
  ///
  /// @tparam Iterator  Input iterator of weights returning double.
  ///
  /// @param[in] first1  The begin of the interval weights.
  /// @param[in] last1  The sentinel end of the interval weights.
  ///
  /// @returns Integer in the range [0, n).
  template <class Iterator>
  static int DiscreteGenerator(Iterator first1, Iterator last1) noexcept {
    return std::discrete_distribution<>(first1, last1)(rng_);
  }

  /// RNG from a Binomial distribution.
  ///
  /// @param[in] n  Number of trials.
  /// @param[in] p  Probability of success.
  ///
  /// @returns The number of successes.
  static int BinomialGenerator(int n, double p) noexcept {
    return std::binomial_distribution<>(n, p)(rng_);
  }

  /// RNG from a normal distribution.
  ///
  /// @param[in] mean  The mean of the distribution.
  /// @param[in] sigma  The standard deviation of the distribution.
  ///
  /// @returns A sampled value.
  static double NormalGenerator(double mean, double sigma) noexcept {
    assert(sigma >= 0);
    return std::normal_distribution<>(mean, sigma)(rng_);
  }

  /// RNG from a lognormal distribution.
  ///
  /// @param[in] m  The m location parameter of the distribution.
  /// @param[in] s  The s scale factor of the distribution.
  ///
  /// @returns A sampled value.
  static double LognormalGenerator(double m, double s) noexcept {
    assert(s >= 0);
    return std::lognormal_distribution<>(m, s)(rng_);
  }

  /// RNG from a Gamma distribution.
  ///
  /// @param[in] k  Shape parameter of Gamma distribution.
  /// @param[in] theta  Scale parameter of Gamma distribution.
  ///
  /// @returns A sampled value.
  ///
  /// @note The rate parameter is 1/theta,
  ///       so for alpha/beta system,
  ///       pass 1/beta as a second parameter for this generator.
  static double GammaGenerator(double k, double theta) noexcept {
    assert(k > 0);
    assert(theta > 0);
    return std::gamma_distribution<>(k)(rng_) * theta;
  }

  /// RNG from a Beta distribution.
  ///
  /// @param[in] alpha  Alpha shape parameter of Beta distribution.
  /// @param[in] beta  Beta shape parameter of Beta distribution.
  ///
  /// @returns A sampled value.
  static double BetaGenerator(double alpha, double beta) noexcept {
    assert(alpha > 0);
    assert(beta > 0);
    return boost::random::beta_distribution<>(alpha, beta)(rng_);
  }

  /// RNG from a Weibull distribution.
  ///
  /// @param[in] k  Shape parameter of Weibull distribution.
  /// @param[in] lambda  Scale parameter of Weibull distribution.
  ///
  /// @returns A sampled value.
  static double WeibullGenerator(double k, double lambda) noexcept {
    assert(k > 0);
    assert(lambda > 0);
    return std::weibull_distribution<>(k, lambda)(rng_);
  }

  /// RNG from an Exponential distribution.
  ///
  /// @param[in] lambda  Rate parameter of Exponential distribution.
  ///
  /// @returns A sampled value.
  static double ExponentialGenerator(double lambda) noexcept {
    assert(lambda > 0);
    return std::exponential_distribution<>(lambda)(rng_);
  }

  /// RNG from a Poisson distribution.
  ///
  /// @param[in] mean  The mean value for Poisson distribution.
  ///
  /// @returns A sampled value.
  static int PoissonGenerator(int mean) noexcept {
    assert(mean > 0);
    return std::poisson_distribution<>(mean)(rng_);
  }

  /// RNG from a log-uniform distribution.
  ///
  /// @param[in] lower  Lower bound.
  /// @param[in] upper  Upper bound.
  ///
  /// @returns A sampled value.
  static double LogUniformGenerator(double lower, double upper) noexcept {
    return std::exp(UniformRealGenerator(lower, upper));
  }

  /// RNG from a log-triangular distribution.
  ///
  /// @param[in] lower  Lower bound.
  /// @param[in] mode  The peak of the distribution.
  /// @param[in] upper  Upper bound.
  ///
  /// @returns A sampled value.
  static double LogTriangularGenerator(double lower, double mode,
                                       double upper) noexcept {
    return std::exp(TriangularGenerator(lower, mode, upper));
  }

 private:
  static std::mt19937 rng_;  ///< The random number generator.
};

}  // namespace scram

#endif  // SCRAM_SRC_RANDOM_H_
