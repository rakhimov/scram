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

/// @file
/// Implementations of random deviate expressions.

#include "random_deviate.h"

#include <cmath>

#include <functional>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/math/special_functions/beta.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/range/algorithm_ext/is_sorted.hpp>

#include "src/error.h"
#include "src/ext/algorithm.h"
#include "src/random.h"

namespace scram {
namespace mef {

UniformDeviate::UniformDeviate(Expression* min, Expression* max)
    : RandomDeviate({min, max}), min_(*min), max_(*max) {}

void UniformDeviate::Validate() const {
  if (min_.value() >= max_.value()) {
    SCRAM_THROW(
        ValidityError("Min value is more than max for Uniform distribution."));
  }
}

double UniformDeviate::DoSample() noexcept {
  return Random::UniformRealGenerator(min_.value(), max_.value());
}

NormalDeviate::NormalDeviate(Expression* mean, Expression* sigma)
    : RandomDeviate({mean, sigma}), mean_(*mean), sigma_(*sigma) {}

void NormalDeviate::Validate() const {
  if (sigma_.value() <= 0) {
    SCRAM_THROW(DomainError("Standard deviation cannot be negative or zero."));
  }
}

double NormalDeviate::DoSample() noexcept {
  return Random::NormalGenerator(mean_.value(), sigma_.value());
}

LognormalDeviate::LognormalDeviate(Expression* mean, Expression* ef,
                                   Expression* level)
    : RandomDeviate({mean, ef, level}),
      flavor_(new LognormalDeviate::Logarithmic(mean, ef, level)) {}

LognormalDeviate::LognormalDeviate(Expression* mu, Expression* sigma)
    : RandomDeviate({mu, sigma}),
      flavor_(new LognormalDeviate::Normal(mu, sigma)) {}

void LognormalDeviate::Logarithmic::Validate() const {
  if (level_.value() <= 0 || level_.value() >= 1) {
    SCRAM_THROW(DomainError("The confidence level is not within (0, 1)."));
  } else if (ef_.value() <= 1) {
    SCRAM_THROW(DomainError(
        "The Error Factor for Log-Normal distribution cannot be less than 1."));
  } else if (mean_.value() <= 0) {
    SCRAM_THROW(DomainError(
        "The mean of Log-Normal distribution cannot be negative or zero."));
  }
}

double LognormalDeviate::DoSample() noexcept {
  return Random::LognormalGenerator(flavor_->location(), flavor_->scale());
}

Interval LognormalDeviate::interval() noexcept {
  double high_estimate = std::exp(3 * flavor_->scale() + flavor_->location());
  return Interval::left_open(0, high_estimate);
}

double LognormalDeviate::Logarithmic::scale() noexcept {
  double z = -std::sqrt(2) * boost::math::erfc_inv(2 * level_.value());
  return std::log(ef_.value()) / z;
}

double LognormalDeviate::Logarithmic::location() noexcept {
  return std::log(mean_.value()) - std::pow(scale(), 2) / 2;
}

void LognormalDeviate::Normal::Validate() const {
  if (sigma_.value() <= 0)
    SCRAM_THROW(DomainError("Standard deviation cannot be negative or zero."));
}

double LognormalDeviate::Normal::mean() noexcept {
  return std::exp(location() + std::pow(scale(), 2) / 2);
}

GammaDeviate::GammaDeviate(Expression* k, Expression* theta)
    : RandomDeviate({k, theta}), k_(*k), theta_(*theta) {}

void GammaDeviate::Validate() const {
  if (k_.value() <= 0) {
    SCRAM_THROW(
        DomainError("The k shape parameter for Gamma distribution"
                    " cannot be negative or zero."));
  } else if (theta_.value() <= 0) {
    SCRAM_THROW(
        DomainError("The theta scale parameter for Gamma distribution"
                    " cannot be negative or zero."));
  }
}

Interval GammaDeviate::interval() noexcept {
  using boost::math::gamma_q;
  double k_max = k_.value();
  double high_estimate =
      theta_.value() * std::pow(gamma_q(k_max, gamma_q(k_max, 0) - 0.99), -1);
  return Interval::left_open(0, high_estimate);
}

double GammaDeviate::DoSample() noexcept {
  return Random::GammaGenerator(k_.value(), theta_.value());
}

BetaDeviate::BetaDeviate(Expression* alpha, Expression* beta)
    : RandomDeviate({alpha, beta}), alpha_(*alpha), beta_(*beta) {}

void BetaDeviate::Validate() const {
  if (alpha_.value() <= 0) {
    SCRAM_THROW(
        DomainError("The alpha shape parameter for Beta distribution"
                    " cannot be negative or zero."));
  } else if (beta_.value() <= 0) {
    SCRAM_THROW(
        DomainError("The beta shape parameter for Beta distribution"
                    " cannot be negative or zero."));
  }
}

Interval BetaDeviate::interval() noexcept {
  double high_estimate =
      std::pow(boost::math::ibeta(alpha_.value(), beta_.value(), 0.99), -1);
  return Interval::closed(0, high_estimate);
}

double BetaDeviate::DoSample() noexcept {
  return Random::BetaGenerator(alpha_.value(), beta_.value());
}

Histogram::Histogram(std::vector<Expression*> boundaries,
                     std::vector<Expression*> weights)
    : RandomDeviate(std::move(boundaries)) {  // Partial registration!
  int num_intervals = Expression::args().size() - 1;
  if (weights.size() != num_intervals) {
    SCRAM_THROW(ValidityError(
        "The number of weights is not equal to the number of intervals."));
  }

  // Complete the argument registration.
  for (Expression* arg : weights)
    Expression::AddArg(arg);

  auto midpoint = std::next(Expression::args().begin(), num_intervals + 1);
  boundaries_ = IteratorRange(Expression::args().begin(), midpoint);
  weights_ = IteratorRange(midpoint, Expression::args().end());
}

void Histogram::Validate() const {
  if (ext::any_of(weights_, [](const auto& expr) { return expr->value() < 0; }))
    SCRAM_THROW(ValidityError("Histogram weights cannot be negative."));

  if (!boost::is_sorted(boundaries_, [](const auto& lhs, const auto& rhs) {
        return lhs->value() <= rhs->value();
      })) {
    SCRAM_THROW(ValidityError(
        "Histogram upper boundaries are not strictly increasing."));
  }
}

double Histogram::value() noexcept {
  double sum_weights = 0;
  double sum_product = 0;
  auto it_b = boundaries_.begin();
  double prev_bound = (*it_b)->value();
  for (const auto& weight : weights_) {
    double cur_weight = weight->value();
    double cur_bound = (*++it_b)->value();
    sum_product += (cur_bound + prev_bound) * cur_weight;
    sum_weights += cur_weight;
    prev_bound = cur_bound;
  }
  return sum_product / (2 * sum_weights);
}

namespace {

/// Provides a helper iterator adaptor for retrieving mean values.
template <class Iterator>
auto make_sampler(const Iterator& it) {
  return boost::make_transform_iterator(it, std::mem_fn(&Expression::value));
}

}  // namespace

double Histogram::DoSample() noexcept {
  return Random::HistogramGenerator(make_sampler(boundaries_.begin()),
                                    make_sampler(boundaries_.end()),
                                    make_sampler(weights_.begin()));
}

}  // namespace mef
}  // namespace scram
