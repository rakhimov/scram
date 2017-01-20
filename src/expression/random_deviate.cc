/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file random_deviate.cc
/// Implementations of random deviate expressions.

#include "random_deviate.h"

#include <cmath>

#include <functional>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/math/special_functions/beta.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/gamma.hpp>

#include "src/error.h"
#include "src/random.h"

namespace scram {
namespace mef {

UniformDeviate::UniformDeviate(const ExpressionPtr& min,
                               const ExpressionPtr& max)
    : RandomDeviate({min, max}),
      min_(*min),
      max_(*max) {}

void UniformDeviate::Validate() const {
  if (min_.Mean() >= max_.Mean()) {
    throw InvalidArgument("Min value is more than max for Uniform"
                          " distribution.");
  } else if (min_.Max() >= max_.Min()) {
    throw InvalidArgument("Sampled min value is more than sampled max"
                          " for Uniform distribution.");
  }
}

double UniformDeviate::DoSample() noexcept {
  return Random::UniformRealGenerator(min_.Sample(), max_.Sample());
}

NormalDeviate::NormalDeviate(const ExpressionPtr& mean,
                             const ExpressionPtr& sigma)
    : RandomDeviate({mean, sigma}),
      mean_(*mean),
      sigma_(*sigma) {}

void NormalDeviate::Validate() const {
  if (sigma_.Mean() <= 0) {
    throw InvalidArgument("Standard deviation cannot be negative or zero.");
  } else if (sigma_.Min() <= 0) {
    throw InvalidArgument("Sampled standard deviation is negative or zero.");
  }
}

double NormalDeviate::DoSample() noexcept {
  return Random::NormalGenerator(mean_.Sample(), sigma_.Sample());
}

LogNormalDeviate::LogNormalDeviate(const ExpressionPtr& mean,
                                   const ExpressionPtr& ef,
                                   const ExpressionPtr& level)
    : RandomDeviate({mean, ef, level}),
      mean_(*mean),
      ef_(*ef),
      level_(*level) {}

void LogNormalDeviate::Validate() const {
  if (level_.Mean() <= 0 || level_.Mean() >= 1) {
    throw InvalidArgument("The confidence level is not within (0, 1).");
  } else if (ef_.Mean() <= 1) {
    throw InvalidArgument("The Error Factor for Log-Normal distribution"
                          " cannot be less than 1.");
  } else if (mean_.Mean() <= 0) {
    throw InvalidArgument("The mean of Log-Normal distribution cannot be"
                          " negative or zero.");
  } else if (level_.Min() <= 0 || level_.Max() >= 1) {
    throw InvalidArgument("The confidence level doesn't sample within (0, 1).");

  } else if (ef_.Min() <= 1) {
    throw InvalidArgument("The Sampled Error Factor for Log-Normal"
                          " distribution cannot be less than 1.");
  } else if (mean_.Min() <= 0) {
    throw InvalidArgument("The sampled mean of Log-Normal distribution"
                          " cannot be negative or zero.");
  }
}

double LogNormalDeviate::DoSample() noexcept {
  double sigma = ComputeScale(level_.Sample(), ef_.Sample());
  double mu = ComputeLocation(mean_.Sample(), sigma);
  return Random::LogNormalGenerator(mu, sigma);
}

double LogNormalDeviate::Max() noexcept {
  double sigma = ComputeScale(level_.Mean(), ef_.Mean());
  double mu = ComputeLocation(mean_.Max(), sigma);
  return std::exp(3 * sigma + mu);
}

double LogNormalDeviate::ComputeScale(double level, double ef) noexcept {
  double z = -std::sqrt(2) * boost::math::erfc_inv(2 * level);
  return std::log(ef) / z;
}

double LogNormalDeviate::ComputeLocation(double mean, double sigma) noexcept {
  return std::log(mean) - std::pow(sigma, 2) / 2;
}

GammaDeviate::GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta)
    : RandomDeviate({k, theta}),
      k_(*k),
      theta_(*theta) {}

void GammaDeviate::Validate() const {
  if (k_.Mean() <= 0) {
    throw InvalidArgument("The k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta_.Mean() <= 0) {
    throw InvalidArgument("The theta scale parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (k_.Min() <= 0) {
    throw InvalidArgument("Sampled k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta_.Min() <= 0) {
    throw InvalidArgument("Sampled theta scale parameter for Gamma "
                          "distribution cannot be negative or zero.");
  }
}

double GammaDeviate::Max() noexcept {
  using boost::math::gamma_q;
  double k_max = k_.Max();
  return theta_.Max() * std::pow(gamma_q(k_max, gamma_q(k_max, 0) - 0.99), -1);
}

double GammaDeviate::DoSample() noexcept {
  return Random::GammaGenerator(k_.Sample(), theta_.Sample());
}

BetaDeviate::BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta)
    : RandomDeviate({alpha, beta}),
      alpha_(*alpha),
      beta_(*beta) {}

void BetaDeviate::Validate() const {
  if (alpha_.Mean() <= 0) {
    throw InvalidArgument("The alpha shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (beta_.Mean() <= 0) {
    throw InvalidArgument("The beta shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (alpha_.Min() <= 0) {
    throw InvalidArgument("Sampled alpha shape parameter for"
                          " Beta distribution cannot be negative or zero.");
  } else if (beta_.Min() <= 0) {
    throw InvalidArgument("Sampled beta shape parameter for Beta"
                          " distribution cannot be negative or zero.");
  }
}

double BetaDeviate::Max() noexcept {
  return std::pow(boost::math::ibeta(alpha_.Max(), beta_.Max(), 0.99), -1);
}

double BetaDeviate::DoSample() noexcept {
  return Random::BetaGenerator(alpha_.Sample(), beta_.Sample());
}

Histogram::Histogram(std::vector<ExpressionPtr> boundaries,
                     std::vector<ExpressionPtr> weights)
    : RandomDeviate(std::move(boundaries)) {  // Partial registration!
  int num_intervals = Expression::args().size() - 1;
  if (weights.size() != num_intervals) {
    throw InvalidArgument("The number of weights is not equal to the number"
                          " of intervals.");
  }

  // Complete the argument registration.
  for (const ExpressionPtr& arg : weights)
    Expression::AddArg(arg);

  auto midpoint = std::next(Expression::args().begin(), num_intervals + 1);
  boundaries_ = IteratorRange(Expression::args().begin(), midpoint);
  weights_ = IteratorRange(midpoint, Expression::args().end());
}

double Histogram::Mean() noexcept {
  double sum_weights = 0;
  double sum_product = 0;
  auto it_b = boundaries_.begin();
  double prev_bound = (*it_b)->Mean();
  for (const auto& weight : weights_) {
    double cur_weight = weight->Mean();
    double cur_bound = (*++it_b)->Mean();
    sum_product += (cur_bound + prev_bound) * cur_weight;
    sum_weights += cur_weight;
    prev_bound = cur_bound;
  }
  return sum_product / (2 * sum_weights);
}

namespace {

/// Provides a helper iterator adaptor for retrieving sampled values.
template <class Iterator>
auto make_sampler(const Iterator& it) {
  return boost::make_transform_iterator(it, std::mem_fn(&Expression::Sample));
}

}  // namespace

double Histogram::DoSample() noexcept {
  return Random::HistogramGenerator(make_sampler(boundaries_.begin()),
                                    make_sampler(boundaries_.end()),
                                    make_sampler(weights_.begin()));
}

void Histogram::CheckBoundaries() const {
  for (auto it = boundaries_.begin(); it != std::prev(boundaries_.end());
       ++it) {
    const auto& prev_expr = *it;
    const auto& cur_expr = *std::next(it);
    if (prev_expr->Mean() >= cur_expr->Mean()) {
      throw InvalidArgument("Histogram upper boundaries are not strictly"
                            " increasing.");
    } else if (prev_expr->Max() >= cur_expr->Min()) {
      throw InvalidArgument("Histogram sampled upper boundaries must"
                            " be strictly increasing.");
    }
  }
}

void Histogram::CheckWeights() const {
  for (const auto& expr : weights_) {
    if (expr->Mean() < 0) {
      throw InvalidArgument("Histogram weights can't be negative.");
    } else if (expr->Min() < 0) {
      throw InvalidArgument("Histogram sampled weights can't be negative.");
    }
  }
}

}  // namespace mef
}  // namespace scram
