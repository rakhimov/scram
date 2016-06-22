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

#include <array>
#include <random>
#include <vector>

namespace scram {

/// This class contains generators for various random distributions.
/// The values passed to the member functions are asserted
/// to be in the correct form.
/// In other words, the user should make sure
/// that the passed parameters are valid.
/// For example, standard deviation cannot be negative.
class Random {
 public:
  /// Sets the seed of the underlying random number generator.
  ///
  /// @param[in] seed  The seed for RNGs.
  static void seed(int seed) noexcept {
    Random::rng_.seed(static_cast<unsigned>(seed));
  }

  /// RNG from uniform distribution.
  ///
  /// @param[in] min  Lower bound.
  /// @param[in] max  Upper bound.
  ///
  /// @returns A sampled value.
  static double UniformRealGenerator(double min, double max) noexcept {
    assert(min < max);
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng_);
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
    static const std::array<double, 3> weights = {0, 1, 0};
    std::array<double, 3> intervals = {lower, mode, upper};
    std::piecewise_linear_distribution<double> dist(intervals.begin(),
                                                    intervals.end(),
                                                    weights.begin());
    return dist(rng_);
  }

  /// RNG from a piecewise linear distribution.
  ///
  /// @param[in] intervals  Interval points for the distribution.
  ///                       The values must be strictly increasing.
  /// @param[in] weights  Weights at the boundaries.
  ///                     The number of weights must be equal to
  ///                     the number of intervals (points - 1).
  ///                     Extra weights are ignored.
  ///
  /// @returns A sampled value.
  static double PiecewiseLinearGenerator(
      const std::vector<double>& intervals,
      const std::vector<double>& weights) noexcept {
    std::piecewise_linear_distribution<double> dist(intervals.begin(),
                                                    intervals.end(),
                                                    weights.begin());
    return dist(rng_);
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
    std::piecewise_constant_distribution<double> dist(first_b, last_b, first_w);
    return dist(rng_);
  }

  /// RNG from a discrete distribution.
  ///
  /// @tparam T  Type of discrete values.
  ///
  /// @param[in] values  Discrete values.
  /// @param[in] weights  Weights for the corresponding values.
  ///                     The size must be the same as the values vector size.
  ///
  /// @returns A sample Value from the value vector.
  template <typename T>
  static T DiscreteGenerator(const std::vector<T>& values,
                             const std::vector<double>& weights) noexcept {
    assert(values.size() == weights.size());
    return values[DiscreteGenerator(weights)];
  }

  /// RNG from Binomial distribution.
  ///
  /// @param[in] n  Number of trials.
  /// @param[in] p  Probability of success.
  ///
  /// @returns The number of successes.
  static int BinomialGenerator(int n, double p) noexcept {
    std::binomial_distribution<int> dist(n, p);
    return dist(rng_);
  }

  /// RNG from a normal distribution.
  ///
  /// @param[in] mean  The mean of the distribution.
  /// @param[in] sigma  The standard deviation of the distribution.
  ///
  /// @returns A sampled value.
  static double NormalGenerator(double mean, double sigma) noexcept {
    assert(sigma >= 0);
    std::normal_distribution<double> dist(mean, sigma);
    return dist(rng_);
  }

  /// RNG from lognormal distribution.
  ///
  /// @param[in] m  The m location parameter of the distribution.
  /// @param[in] s  The s scale factor of the distribution.
  ///
  /// @returns A sampled value.
  static double LogNormalGenerator(double m, double s) noexcept {
    assert(s >= 0);
    std::lognormal_distribution<double> dist(m, s);
    return dist(rng_);
  }

  /// RNG from Gamma distribution.
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
    std::gamma_distribution<double> gamma_dist(k);
    return theta * gamma_dist(rng_);
  }

  /// RNG from Beta distribution.
  ///
  /// @param[in] alpha  Alpha shape parameter of Beta distribution.
  /// @param[in] beta  Beta shape parameter of Beta distribution.
  ///
  /// @returns A sampled value.
  static double BetaGenerator(double alpha, double beta) noexcept {
    assert(alpha > 0);
    assert(beta > 0);
    std::gamma_distribution<double> gamma_dist_x(alpha);
    std::gamma_distribution<double> gamma_dist_y(beta);
    double x = gamma_dist_x(rng_);
    double y = gamma_dist_y(rng_);
    return x / (x + y);
  }

  /// RNG from Weibull distribution.
  ///
  /// @param[in] k  Shape parameter of Weibull distribution.
  /// @param[in] lambda  Scale parameter of Weibull distribution.
  ///
  /// @returns A sampled value.
  static double WeibullGenerator(double k, double lambda) noexcept {
    assert(k > 0);
    assert(lambda > 0);
    std::weibull_distribution<double> dist(k, lambda);
    return dist(rng_);
  }

  /// RNG from Exponential distribution.
  ///
  /// @param[in] lambda  Rate parameter of Exponential distribution.
  ///
  /// @returns A sampled value.
  static double ExponentialGenerator(double lambda) noexcept {
    assert(lambda > 0);
    std::exponential_distribution<double> dist(lambda);
    return dist(rng_);
  }

  /// RNG from Poisson distribution.
  ///
  /// @param[in] mean  The mean value for Poisson distribution.
  ///
  /// @returns A sampled value.
  static int PoissonGenerator(int mean) noexcept {
    assert(mean > 0);
    std::poisson_distribution<int> dist(mean);
    return dist(rng_);
  }

  /// RNG from log-uniform distribution.
  ///
  /// @param[in] min  Lower bound.
  /// @param[in] max  Upper bound.
  ///
  /// @returns A sampled value.
  static double LogUniformGenerator(double min, double max) noexcept {
    return std::exp(Random::UniformRealGenerator(min, max));
  }

  /// RNG from log-triangular distribution.
  ///
  /// @param[in] lower  Lower bound.
  /// @param[in] mode  The peak of the distribution.
  /// @param[in] upper  Upper bound.
  ///
  /// @returns A sampled value.
  static double LogTriangularGenerator(double lower, double mode,
                                       double upper) noexcept {
    return std::exp(Random::TriangularGenerator(lower, mode, upper));
  }

 private:
  /// RNG from a discrete distribution.
  ///
  /// @param[in] weights  Weights for the range [0, n),
  ///                     where n is the size of the vector.
  ///
  /// @returns Integer in the range [0, n).
  static int DiscreteGenerator(const std::vector<double>& weights) noexcept {
    std::discrete_distribution<int> dist(weights.begin(), weights.end());
    return dist(rng_);
  }

  static std::mt19937 rng_;  ///< The random number generator.
};

}  // namespace scram

#endif  // SCRAM_SRC_RANDOM_H_
