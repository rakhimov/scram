/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

/// @file random.cc
/// Implementation for various RNGs.

#include "random.h"

#include <array>
#include <cmath>

namespace scram {

std::mt19937 Random::rng_;

void Random::seed(int seed) noexcept {
  Random::rng_.seed(static_cast<unsigned>(seed));
}

double Random::UniformRealGenerator(double min, double max) noexcept {
  assert(min < max);
  std::uniform_real_distribution<double> dist(min, max);
  return dist(rng_);
}

double Random::TriangularGenerator(double lower, double mode,
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

double Random::PiecewiseLinearGenerator(
    const std::vector<double>& intervals,
    const std::vector<double>& weights) noexcept {
  std::piecewise_linear_distribution<double> dist(intervals.begin(),
                                                  intervals.end(),
                                                  weights.begin());
  return dist(rng_);
}

double Random::HistogramGenerator(
    const std::vector<double>& intervals,
    const std::vector<double>& weights) noexcept {
  std::piecewise_constant_distribution<double> dist(intervals.begin(),
                                                    intervals.end(),
                                                    weights.begin());
  return dist(rng_);
}

int Random::DiscreteGenerator(const std::vector<double>& weights) noexcept {
  std::discrete_distribution<int> dist(weights.begin(), weights.end());
  return dist(rng_);
}

int Random::BinomialGenerator(int n, double p) noexcept {
  std::binomial_distribution<int> dist(n, p);
  return dist(rng_);
}

double Random::NormalGenerator(double mean, double sigma) noexcept {
  assert(sigma >= 0);
  std::normal_distribution<double> dist(mean, sigma);
  return dist(rng_);
}

double Random::LogNormalGenerator(double m, double s) noexcept {
  assert(s >= 0);
  std::lognormal_distribution<double> dist(m, s);
  return dist(rng_);
}

double Random::GammaGenerator(double k, double theta) noexcept {
  assert(k > 0);
  assert(theta > 0);
  std::gamma_distribution<double> gamma_dist(k);
  return theta * gamma_dist(rng_);
}

double Random::BetaGenerator(double alpha, double beta) noexcept {
  assert(alpha > 0);
  assert(beta > 0);
  std::gamma_distribution<double> gamma_dist_x(alpha);
  std::gamma_distribution<double> gamma_dist_y(beta);

  double x = gamma_dist_x(rng_);
  double y = gamma_dist_y(rng_);

  return x / (x + y);
}

double Random::WeibullGenerator(double k, double lambda) noexcept {
  assert(k > 0);
  assert(lambda > 0);
  std::weibull_distribution<double> dist(k, lambda);
  return dist(rng_);
}

double Random::ExponentialGenerator(double lambda) noexcept {
  assert(lambda > 0);
  std::exponential_distribution<double> dist(lambda);
  return dist(rng_);
}

int Random::PoissonGenerator(int mean) noexcept {
  assert(mean > 0);
  std::poisson_distribution<int> dist(mean);
  return dist(rng_);
}

double Random::LogUniformGenerator(double min, double max) noexcept {
  return std::exp(Random::UniformRealGenerator(min, max));
}

double Random::LogTriangularGenerator(double lower, double mode,
                                      double upper) noexcept {
  return std::exp(Random::TriangularGenerator(lower, mode, upper));
}

}  // namespace scram
