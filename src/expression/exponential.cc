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

/// @file exponential.cc
/// Implementation of various exponential expressions.

#include "exponential.h"

#include <algorithm>

#include "src/error.h"

namespace scram {
namespace mef {

ExponentialExpression::ExponentialExpression(const ExpressionPtr& lambda,
                                             const ExpressionPtr& t)
    : Expression({lambda, t}),
      lambda_(*lambda),
      time_(*t) {}

void ExponentialExpression::Validate() const {
  if (lambda_.Mean() < 0) {
    throw InvalidArgument("The rate of failure cannot be negative.");
  } else if (time_.Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (lambda_.Min() < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (time_.Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }
}

GlmExpression::GlmExpression(const ExpressionPtr& gamma,
                             const ExpressionPtr& lambda,
                             const ExpressionPtr& mu,
                             const ExpressionPtr& t)
    : Expression({gamma, lambda, mu, t}),
      gamma_(*gamma),
      lambda_(*lambda),
      mu_(*mu),
      time_(*t) {}

void GlmExpression::Validate() const {
  if (lambda_.Mean() < 0) {
    throw InvalidArgument("The rate of failure cannot be negative.");
  } else if (mu_.Mean() < 0) {
    throw InvalidArgument("The rate of repair cannot be negative.");
  } else if (gamma_.Mean() < 0 || gamma_.Mean() > 1) {
    throw InvalidArgument("Invalid value for probability.");
  } else if (time_.Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (lambda_.Min() < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (mu_.Min() < 0) {
    throw InvalidArgument("The sampled rate of repair cannot be negative.");
  } else if (gamma_.Min() < 0 || gamma_.Max() > 1) {
    throw InvalidArgument("Invalid sampled gamma value for probability.");
  } else if (time_.Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }
}

double GlmExpression::Mean() noexcept {
  return Compute(gamma_.Mean(), lambda_.Mean(), mu_.Mean(), time_.Mean());
}

double GlmExpression::DoSample() noexcept {
  return Compute(gamma_.Sample(), lambda_.Sample(), mu_.Sample(),
                 time_.Sample());
}

double GlmExpression::Compute(double gamma, double lambda, double mu,
                              double time) noexcept {
  double r = lambda + mu;
  return (lambda - (lambda - gamma * r) * std::exp(-r * time)) / r;
}

WeibullExpression::WeibullExpression(const ExpressionPtr& alpha,
                                     const ExpressionPtr& beta,
                                     const ExpressionPtr& t0,
                                     const ExpressionPtr& time)
    : Expression({alpha, beta, t0, time}),
      alpha_(*alpha),
      beta_(*beta),
      t0_(*t0),
      time_(*time) {}

void WeibullExpression::Validate() const {
  if (alpha_.Mean() <= 0) {
    throw InvalidArgument("The scale parameter for Weibull distribution must"
                          " be positive.");
  } else if (beta_.Mean() <= 0) {
    throw InvalidArgument("The shape parameter for Weibull distribution must"
                          " be positive.");
  } else if (t0_.Mean() < 0) {
    throw InvalidArgument("Invalid value for time shift.");
  } else if (time_.Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (time_.Mean() < t0_.Mean()) {
    throw InvalidArgument("The mission time must be longer than time shift.");
  } else if (alpha_.Min() <= 0) {
    throw InvalidArgument("The scale parameter for Weibull distribution must"
                          " be positive for sampled values.");
  } else if (beta_.Min() <= 0) {
    throw InvalidArgument("The shape parameter for Weibull distribution must"
                          " be positive for sampled values.");
  } else if (t0_.Min() < 0) {
    throw InvalidArgument("Invalid value for time shift in sampled values.");
  } else if (time_.Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  } else if (time_.Min() < t0_.Max()) {
    throw InvalidArgument("The sampled mission time must be"
                          " longer than time shift.");
  }
}

double WeibullExpression::Compute(double alpha, double beta,
                                  double t0, double time) noexcept {
  return 1 - std::exp(-std::pow((time - t0) / alpha, beta));
}

PeriodicTest::PeriodicTest(const ExpressionPtr& lambda,
                           const ExpressionPtr& tau, const ExpressionPtr& theta,
                           const ExpressionPtr& time)
    : Expression({lambda, tau, theta, time}),
      flavor_(new PeriodicTest::InstantRepair(lambda, tau, theta, time)) {}

/// Checks and throws if an expression is negative.
#define THROW_NEGATIVE_EXPR(expr, description)                          \
  do {                                                                  \
    if (expr.Mean() < 0)                                                \
      throw InvalidArgument("The " description " cannot be negative."); \
    if (expr.Min() < 0)                                                 \
      throw InvalidArgument("The sampled " description                  \
                            " cannot be negative.");                    \
  } while (false)

void PeriodicTest::InstantRepair::Validate() const {
  THROW_NEGATIVE_EXPR(lambda_, "rate of failure");
  THROW_NEGATIVE_EXPR(tau_, "time between tests");
  THROW_NEGATIVE_EXPR(theta_, "time before tests");
  THROW_NEGATIVE_EXPR(time_, "mission time");
}

#undef THROW_NEGATIVE_EXPR

namespace {

/// Negative exponential law probability.
double p_exp(double lambda, double time) {
  return 1 - std::exp(-lambda * time);
}

}  // namespace

double PeriodicTest::InstantRepair::Compute(double lambda, double tau,
                                            double theta,
                                            double time) noexcept {
  if (time <= theta)
    return p_exp(lambda, time);
  double delta = time - theta;
  double time_after_test = delta - static_cast<int>(delta / tau) * tau;
  return p_exp(lambda, time_after_test ? time_after_test : tau);
}

double PeriodicTest::InstantRepair::Mean() noexcept {
  return Compute(lambda_.Mean(), tau_.Mean(), theta_.Mean(), time_.Mean());
}

double PeriodicTest::InstantRepair::Max() noexcept {
  return std::max(p_exp(lambda_.Max(), theta_.Max()),
                  p_exp(lambda_.Max(), tau_.Max()));
}

double PeriodicTest::InstantRepair::Min() noexcept {
  return std::min(p_exp(lambda_.Min(), theta_.Min()),
                  p_exp(lambda_.Min(), tau_.Min()));
}

double PeriodicTest::InstantRepair::Sample() noexcept {
  return Compute(lambda_.Sample(), tau_.Sample(), theta_.Sample(),
                 time_.Sample());
}

}  // namespace mef
}  // namespace scram
