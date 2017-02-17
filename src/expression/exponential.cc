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

#include <cmath>

#include "src/error.h"

namespace scram {
namespace mef {

/// Checks and throws if an expression is negative.
#define THROW_NEGATIVE_EXPR(expr, description)                          \
  do {                                                                  \
    if (expr.Mean() < 0)                                                \
      throw InvalidArgument("The " description " cannot be negative."); \
    if (expr.Min() < 0)                                                 \
      throw InvalidArgument("The sampled " description                  \
                            " cannot be negative.");                    \
  } while (false)

/// Check and throws if an expression is negative or 0.
#define THROW_NON_POSITIVE_EXPR(expr, description)                            \
  do {                                                                        \
    if (expr.Mean() <= 0)                                                     \
      throw InvalidArgument("The " description " must be positive.");         \
    if (expr.Min() <= 0)                                                      \
      throw InvalidArgument("The sampled " description " must be positive."); \
  } while (false)

namespace {

/// Negative exponential law probability.
double p_exp(double lambda, double time) {
  return 1 - std::exp(-lambda * time);
}

}  // namespace

ExponentialExpression::ExponentialExpression(const ExpressionPtr& lambda,
                                             const ExpressionPtr& t)
    : Expression({lambda, t}),
      lambda_(*lambda),
      time_(*t) {}

void ExponentialExpression::Validate() const {
  THROW_NEGATIVE_EXPR(lambda_, "rate of failure");
  THROW_NEGATIVE_EXPR(time_, "mission time");
}

double ExponentialExpression::Mean() noexcept {
  return p_exp(lambda_.Mean(), time_.Mean());
}

double ExponentialExpression::DoSample() noexcept {
  return p_exp(lambda_.Sample(), time_.Sample());
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
  THROW_NON_POSITIVE_EXPR(lambda_, "rate of failure");
  THROW_NEGATIVE_EXPR(mu_, "rate of repair");
  THROW_NEGATIVE_EXPR(time_, "mission time");
  if (gamma_.Mean() < 0 || gamma_.Mean() > 1) {
    throw InvalidArgument("Invalid value for probability.");
  } else if (gamma_.Min() < 0 || gamma_.Max() > 1) {
    throw InvalidArgument("Invalid sampled gamma value for probability.");
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
  THROW_NON_POSITIVE_EXPR(alpha_, "scale parameter for Weibull distribution");
  THROW_NON_POSITIVE_EXPR(beta_, "shape parameter for Weibull distribution");
  THROW_NEGATIVE_EXPR(t0_, "time shift");
  THROW_NEGATIVE_EXPR(time_, "mission time");

  if (time_.Mean() < t0_.Mean()) {
    throw InvalidArgument("The mission time must be longer than time shift.");
  } else if (time_.Min() < t0_.Max()) {
    throw InvalidArgument("The sampled mission time must be"
                          " longer than time shift.");
  }
}

double WeibullExpression::Compute(double alpha, double beta,
                                  double t0, double time) noexcept {
  return 1 - std::exp(-std::pow((time - t0) / alpha, beta));
}

double WeibullExpression::Mean() noexcept {
  return Compute(alpha_.Mean(), beta_.Mean(), t0_.Mean(), time_.Mean());
}

double WeibullExpression::DoSample() noexcept {
  return Compute(alpha_.Sample(), beta_.Sample(), t0_.Sample(), time_.Sample());
}

PeriodicTest::PeriodicTest(const ExpressionPtr& lambda,
                           const ExpressionPtr& tau, const ExpressionPtr& theta,
                           const ExpressionPtr& time)
    : Expression({lambda, tau, theta, time}),
      flavor_(new PeriodicTest::InstantRepair(lambda, tau, theta, time)) {}

PeriodicTest::PeriodicTest(const ExpressionPtr& lambda, const ExpressionPtr& mu,
                           const ExpressionPtr& tau, const ExpressionPtr& theta,
                           const ExpressionPtr& time)
    : Expression({lambda, mu, tau, theta, time}),
      flavor_(new PeriodicTest::InstantTest(lambda, mu, tau, theta, time)) {}

void PeriodicTest::InstantRepair::Validate() const {
  THROW_NON_POSITIVE_EXPR(lambda_, "rate of failure");
  THROW_NON_POSITIVE_EXPR(tau_, "time between tests");
  THROW_NEGATIVE_EXPR(theta_, "time before tests");
  THROW_NEGATIVE_EXPR(time_, "mission time");
}

void PeriodicTest::InstantTest::Validate() const {
  InstantRepair::Validate();
  THROW_NEGATIVE_EXPR(mu_, "rate of repair");
}

#undef THROW_NEGATIVE_EXPR

double PeriodicTest::InstantRepair::Compute(double lambda, double tau,
                                            double theta,
                                            double time) noexcept {
  if (time <= theta)  // No test has been performed.
    return p_exp(lambda, time);
  double delta = time - theta;
  double time_after_test = delta - static_cast<int>(delta / tau) * tau;
  return p_exp(lambda, time_after_test ? time_after_test : tau);
}

double PeriodicTest::InstantRepair::Mean() noexcept {
  return Compute(lambda_.Mean(), tau_.Mean(), theta_.Mean(), time_.Mean());
}

double PeriodicTest::InstantRepair::Sample() noexcept {
  return Compute(lambda_.Sample(), tau_.Sample(), theta_.Sample(),
                 time_.Sample());
}

double PeriodicTest::InstantTest::Compute(double lambda, double mu, double tau,
                                          double theta, double time) noexcept {
  if (time <= theta)  // No test has been performed.
    return p_exp(lambda, time);

  // Carry fraction from probability of a previous period.
  auto carry = [&lambda, &mu](double p_lambda, double p_mu, double t) {
    // Probability of failure after repair.
    double p_mu_lambda = lambda == mu
                             ? p_lambda - (1 - p_lambda) * lambda * t
                             : (lambda * p_mu - mu * p_lambda) / (lambda - mu);
    return 1 - p_mu + p_mu_lambda - p_lambda;
  };

  double prob = p_exp(lambda, theta);  // The current rolling probability.
  auto p_period = [&prob, &carry](double p_lambda, double p_mu, double t) {
    return prob * carry(p_lambda, p_mu, t) + p_lambda;
  };

  double delta = time - theta;
  int num_periods = delta / tau;
  double fraction = carry(p_exp(lambda, tau), p_exp(mu, tau), tau);
  double compound = std::pow(fraction, num_periods);  // Geometric progression.
  prob = prob * compound + p_exp(lambda, tau) * (compound - 1) / (fraction - 1);

  double time_after_test = delta - num_periods * tau;
  return p_period(p_exp(lambda, time_after_test), p_exp(mu, time_after_test),
                  time_after_test);
}

double PeriodicTest::InstantTest::Mean() noexcept {
  return Compute(lambda_.Mean(), mu_.Mean(), tau_.Mean(), theta_.Mean(),
                 time_.Mean());
}

double PeriodicTest::InstantTest::Sample() noexcept {
  return Compute(lambda_.Sample(), mu_.Sample(), tau_.Sample(), theta_.Sample(),
                 time_.Sample());
}

}  // namespace mef
}  // namespace scram
