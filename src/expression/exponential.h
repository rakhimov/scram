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

#include <memory>

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
  double Mean() noexcept override;
  double Max() noexcept override { return 1; }
  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  Expression& lambda_;  ///< Failure rate in hours.
  Expression& time_;  ///< Mission time in hours.
};

/// Exponential with probability of failure on demand,
/// hourly failure rate, hourly repairing rate, and time.
class GlmExpression : public Expression {
 public:
  /// Constructor for GLM or exponential expression with four arguments.
  ///
  /// @param[in] gamma  Probability of failure on demand.
  /// @param[in] lambda  Hourly rate of failure.
  /// @param[in] mu  Hourly repair rate.
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
  double Mean() noexcept override;
  double Max() noexcept override { return 1; }
  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  /// Calculates Weibull expression.
  double Compute(double alpha, double beta, double t0, double time) noexcept;

  Expression& alpha_;  ///< Scale parameter.
  Expression& beta_;  ///< Shape parameter.
  Expression& t0_;  ///< Time shift in hours.
  Expression& time_;  ///< Mission time in hours.
};

/// Periodic test with 3 phases: deploy, test, functioning.
class PeriodicTest : public Expression {
 public:
  /// Periodic tests with tests and repairs instantaneous and always successful.
  ///
  /// @param[in] lambda  The failure rate (hourly) when functioning.
  /// @param[in] tau  The time between tests in hours.
  /// @param[in] theta  The time before the first test in hours.
  /// @param[in] time  The current mission time in hours.
  PeriodicTest(const ExpressionPtr& lambda, const ExpressionPtr& tau,
               const ExpressionPtr& theta, const ExpressionPtr& time);

  /// Periodic tests with tests instantaneous and always successful.
  /// @copydetails PeriodicTest(const ExpressionPtr&, const ExpressionPtr&,
  ///                           const ExpressionPtr&, const ExpressionPtr&)
  ///
  /// @param[in] mu  The repair rate (hourly).
  PeriodicTest(const ExpressionPtr& lambda, const ExpressionPtr& mu,
               const ExpressionPtr& tau, const ExpressionPtr& theta,
               const ExpressionPtr& time);

  /// Fully parametrized periodic-test description.
  /// @copydetails PeriodicTest(const ExpressionPtr&, const ExpressionPtr&,
  ///                           const ExpressionPtr&, const ExpressionPtr&,
  ///                           const ExpressionPtr&)
  ///
  /// @param[in] lambda_test  The component failure rate while under test.
  /// @param[in] gamma  The failure probability due to or at test start.
  /// @param[in] test_duration  The duration of the test phase.
  /// @param[in] available_at_test  Indicator of component availability at test.
  /// @param[in] sigma  The probability of failure detection upon test.
  /// @param[in] omega  The probability of failure at restart after repair/test.
  PeriodicTest(const ExpressionPtr& lambda, const ExpressionPtr& lambda_test,
               const ExpressionPtr& mu, const ExpressionPtr& tau,
               const ExpressionPtr& theta, const ExpressionPtr& gamma,
               const ExpressionPtr& test_duration,
               const ExpressionPtr& available_at_test,
               const ExpressionPtr& sigma, const ExpressionPtr& omega,
               const ExpressionPtr& time);

  void Validate() const override { flavor_->Validate(); }
  double Mean() noexcept override { return flavor_->Mean(); }
  double Max() noexcept override { return 1; }
  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override { return flavor_->Sample(); }

  /// The base class for various flavors of periodic-test computation.
  struct Flavor {
    virtual ~Flavor() = default;
    /// @copydoc Expression::Validate
    virtual void Validate() const = 0;
    /// @copydoc Expression::Mean
    virtual double Mean() noexcept = 0;
    /// @copydoc Expression::Sample
    virtual double Sample() noexcept = 0;
  };

  /// The tests and repairs are instantaneous and always successful.
  class InstantRepair : public Flavor {
   public:
    /// The same semantics as for 4 argument periodic-test.
    InstantRepair(const ExpressionPtr& lambda, const ExpressionPtr& tau,
                  const ExpressionPtr& theta, const ExpressionPtr& time)
        : lambda_(*lambda), tau_(*tau), theta_(*theta), time_(*time) {}

    void Validate() const override;
    double Mean() noexcept override;
    double Sample() noexcept override;

   protected:
    Expression& lambda_;  ///< The failure rate when functioning.
    Expression& tau_;  ///< The time between tests in hours.
    Expression& theta_;  ///< The time before the first test.
    Expression& time_;  ///< The current time.

   private:
    /// Computes the expression value.
    double Compute(double lambda, double tau, double theta,
                   double time) noexcept;
  };

  /// The tests are instantaneous and always successful,
  /// but repairs are not.
  class InstantTest : public InstantRepair {
   public:
    /// The same semantics as for 5 argument periodic-test.
    InstantTest(const ExpressionPtr& lambda, const ExpressionPtr& mu,
                const ExpressionPtr& tau, const ExpressionPtr& theta,
                const ExpressionPtr& time)
        : InstantRepair(lambda, tau, theta, time), mu_(*mu) {}

    void Validate() const override;
    double Mean() noexcept override;
    double Sample() noexcept override;

   protected:
    Expression& mu_;  ///< The repair rate.

   private:
    /// Computes the expression value.
    double Compute(double lambda, double mu, double tau, double theta,
                   double time) noexcept;
  };

  /// The full representation of periodic test with 11 arguments.
  class Complete : public InstantTest {
   public:
    /// The parameters have the same semantics as 11 argument periodic-test.
    Complete(const ExpressionPtr& lambda, const ExpressionPtr& lambda_test,
             const ExpressionPtr& mu, const ExpressionPtr& tau,
             const ExpressionPtr& theta, const ExpressionPtr& gamma,
             const ExpressionPtr& test_duration,
             const ExpressionPtr& available_at_test, const ExpressionPtr& sigma,
             const ExpressionPtr& omega, const ExpressionPtr& time)
        : InstantTest(lambda, mu, tau, theta, time),
          lambda_test_(*lambda_test),
          gamma_(*gamma),
          test_duration_(*test_duration),
          available_at_test_(*available_at_test),
          sigma_(*sigma),
          omega_(*omega) {}

    void Validate() const override;
    double Mean() noexcept override;
    double Sample() noexcept override;

   private:
    /// @returns The expression value.
    /// @copydetails PeriodicTest(const ExpressionPtr&, const ExpressionPtr&,
    ///                           const ExpressionPtr&, const ExpressionPtr&,
    ///                           const ExpressionPtr&, const ExpressionPtr&,
    ///                           const ExpressionPtr&,
    ///                           const ExpressionPtr&, const ExpressionPtr&,
    ///                           const ExpressionPtr&, const ExpressionPtr&)
    double Compute(double lambda, double lambda_test, double mu, double tau,
                   double theta, double gamma, double test_duration,
                   bool available_at_test, double sigma, double omega,
                   double time) noexcept;

    Expression& lambda_test_;  ///< The failure rate while under test.
    Expression& gamma_;  ///< The failure probability due to or at test start.
    Expression& test_duration_;  ///< The duration of the test phase.
    Expression& available_at_test_;  ///< The indicator of availability at test.
    Expression& sigma_;  ///< The probability of failure detection upon test.
    Expression& omega_;  ///< The probability of failure at restart.
  };

  std::unique_ptr<Flavor> flavor_;  ///< Specialized flavor of calculations.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_EXPONENTIAL_H_
