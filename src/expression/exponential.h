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

/// @file exponential.h
/// Expressions and distributions
/// that are described with exponential formulas.

#ifndef SCRAM_SRC_EXPRESSION_EXPONENTIAL_H_
#define SCRAM_SRC_EXPRESSION_EXPONENTIAL_H_

#include <cmath>

#include "src/expression.h"

namespace scram {
namespace mef {

/// Negative exponential distribution
/// with hourly failure rate and time.
class ExponentialExpression : public Expression {
 public:
  /// Constructor for exponential expression with two arguments.
  ///
  /// @param[in] lambda  Hourly rate of failure.
  /// @param[in] t  Mission time in hours.
  ExponentialExpression(const ExpressionPtr& lambda, const ExpressionPtr& t);

  /// @throws InvalidArgument  The failure rate or time is negative.
  void Validate() const override;

  double Mean() noexcept override {
    return 1 - std::exp(-(lambda_.Mean() * time_.Mean()));
  }

  double Max() noexcept override {
    return 1 - std::exp(-(lambda_.Max() * time_.Max()));
  }

  double Min() noexcept override {
    return 1 - std::exp(-(lambda_.Min() * time_.Min()));
  }

 private:
  double DoSample() noexcept override {
    return 1 - std::exp(-(lambda_.Sample() * time_.Sample()));
  }

  Expression& lambda_;  ///< Failure rate in hours.
  Expression& time_;  ///< Mission time in hours.
};

/// Exponential with probability of failure on demand,
/// hourly failure rate, hourly repairing rate, and time.
///
/// @todo Find the minimum and maximum values.
class GlmExpression : public Expression {
 public:
  /// Constructor for GLM or exponential expression with four arguments.
  ///
  /// @param[in] gamma  Probability of failure on demand.
  /// @param[in] lambda  Hourly rate of failure.
  /// @param[in] mu  Hourly repairing rate.
  /// @param[in] t  Mission time in hours.
  GlmExpression(const ExpressionPtr& gamma, const ExpressionPtr& lambda,
                const ExpressionPtr& mu, const ExpressionPtr& t);

  void Validate() const override;

  double Mean() noexcept override;
  double Max() noexcept override { return 1; }
  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  /// Computes the value for GLM expression.
  ///
  /// @param[in] gamma  Value for probability on demand.
  /// @param[in] lambda  Value for hourly rate of failure.
  /// @param[in] mu  Value for hourly repair rate.
  /// @param[in] time  Mission time in hours.
  ///
  /// @returns Probability of failure on demand.
  double Compute(double gamma, double lambda, double mu, double time) noexcept;

  Expression& gamma_;  ///< Probability of failure on demand.
  Expression& lambda_;  ///< Failure rate in hours.
  Expression& mu_;  ///< Repair rate in hours.
  Expression& time_;  ///< Mission time in hours.
};

/// Weibull distribution with scale, shape, time shift, and time.
class WeibullExpression : public Expression {
 public:
  /// Constructor for Weibull distribution.
  ///
  /// @param[in] alpha  Scale parameter.
  /// @param[in] beta  Shape parameter.
  /// @param[in] t0  Time shift.
  /// @param[in] time  Mission time.
  WeibullExpression(const ExpressionPtr& alpha, const ExpressionPtr& beta,
                    const ExpressionPtr& t0, const ExpressionPtr& time);

  void Validate() const override;

  double Mean() noexcept override {
    return Compute(alpha_.Mean(), beta_.Mean(), t0_.Mean(), time_.Mean());
  }

  double Max() noexcept override {
    return Compute(alpha_.Min(), beta_.Max(), t0_.Min(), time_.Max());
  }

  double Min() noexcept override {
    return Compute(alpha_.Max(), beta_.Min(), t0_.Max(), time_.Min());
  }

 private:
  double DoSample() noexcept override {
    return Compute(alpha_.Sample(), beta_.Sample(), t0_.Sample(),
                   time_.Sample());
  }

  /// Calculates Weibull expression.
  ///
  /// @param[in] alpha  Scale parameter.
  /// @param[in] beta  Shape parameter.
  /// @param[in] t0  Time shift.
  /// @param[in] time  Mission time.
  ///
  /// @returns Calculated value.
  double Compute(double alpha, double beta, double t0, double time) noexcept;

  Expression& alpha_;  ///< Scale parameter.
  Expression& beta_;  ///< Shape parameter.
  Expression& t0_;  ///< Time shift in hours.
  Expression& time_;  ///< Mission time in hours.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_EXPONENTIAL_H_
