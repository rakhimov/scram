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

/// @file expression.h
/// Expressions that describe basic events.

#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <cmath>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <boost/math/special_functions/beta.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/gamma.hpp>

#include "element.h"

namespace scram {
namespace mef {

class Expression;
using ExpressionPtr = std::shared_ptr<Expression>;  ///< Shared expressions.

class Parameter;  // This is for cycle detection through expressions.
using ParameterPtr = std::shared_ptr<Parameter>;  ///< Shared parameters.

/// @class Expression
/// Abstract base class for all sorts of expressions to describe events.
/// This class also acts like a connector for parameter nodes
/// and may create cycles.
/// Expressions are not expected to be shared
/// except for parameters.
/// In addition, expressions are not expected to be changed
/// after validation phases.
class Expression {
 public:
  /// Constructor for use by derived classes
  /// to register their arguments.
  ///
  /// @param[in] args  Arguments of this expression.
  explicit Expression(std::vector<ExpressionPtr> args);

  Expression(const Expression&) = delete;
  Expression& operator=(const Expression&) = delete;

  virtual ~Expression() = default;

  /// Validates the expression.
  /// This late validation is due to parameters that are defined late.
  ///
  /// @throws InvalidArgument  The arguments are invalid for setup.
  virtual void Validate() {}

  /// @returns The mean value of this expression.
  virtual double Mean() noexcept = 0;

  /// @returns A sampled value of this expression.
  double Sample() noexcept;

  /// This routine resets the sampling to get new values.
  /// All the arguments are called to reset themselves.
  /// If this expression was not sampled,
  /// its arguments are not going to get any calls.
  virtual void Reset() noexcept;

  /// Determines if the value of the expression varies.
  /// The default logic is to check arguments with uncertainties for sampling.
  /// Derived expression classes must decide
  /// if they don't have arguments,
  /// or if they are random deviates.
  ///
  /// @returns true if the expression's value does not need sampling.
  /// @returns false if the expression's value has uncertainties.
  ///
  /// @warning Improper registration of arguments
  ///          may yield silent failure.
  virtual bool IsConstant() noexcept;

  /// @returns Maximum value of this expression.
  virtual double Max() noexcept { return this->Mean(); }

  /// @returns Minimum value of this expression.
  virtual double Min() noexcept { return this->Mean(); }

  /// @returns Parameters as nodes.
  const std::vector<Parameter*>& nodes() {
    if (gather_) Expression::GatherNodesAndConnectors();
    return nodes_;
  }

  /// @returns Non-Parameter Expressions as connectors.
  const std::vector<Expression*>& connectors() {
    if (gather_) Expression::GatherNodesAndConnectors();
    return connectors_;
  }

 protected:
  /// @returns A set of arguments of the expression.
  const std::vector<ExpressionPtr>& args() const { return args_; }

  /// Registers an additional argument expression.
  ///
  /// @param[in] arg  An argument expression used by this expression.
  void AddArg(const ExpressionPtr& arg) { args_.push_back(arg); }

 private:
  /// Runs sampling of the expression.
  /// Derived concrete classes must provide the calculation.
  ///
  /// @returns A sampled value of this expression.
  virtual double GetSample() noexcept = 0;

  /// Gathers nodes and connectors from arguments of the expression.
  void GatherNodesAndConnectors();

  std::vector<ExpressionPtr> args_;  ///< Expression's arguments.
  double sampled_value_;  ///< The sampled value.
  bool sampled_;  ///< Indication if the expression is already sampled.
  bool gather_;  ///< A flag to gather nodes and connectors.
  std::vector<Parameter*> nodes_;  ///< Parameters as nodes.
  std::vector<Expression*> connectors_;  ///< Expressions as connectors.
};

/// @enum Units
/// Provides units for parameters.
enum Units {
  kUnitless = 0,
  kBool,
  kInt,
  kFloat,
  kHours,
  kInverseHours,
  kYears,
  kInverseYears,
  kFit,
  kDemands
};

/// @class Parameter
/// This class provides a representation of a variable
/// in basic event description.
/// It is both expression and element description.
class Parameter : public Expression, public Element, public Role {
 public:
  /// Creates a parameter as a variable for future references.
  ///
  /// @param[in] name  The name of this variable (Case sensitive).
  /// @param[in] base_path  The series of containers to get this parameter.
  /// @param[in] is_public  Whether or not the parameter is public.
  ///
  /// @throws LogicError  The name is empty.
  explicit Parameter(const std::string& name,
                     const std::string& base_path = "",
                     bool is_public = true);

  /// Sets the expression of this parameter.
  ///
  /// @param[in] expression  The expression to describe this parameter.
  ///
  /// @throws LogicError  The parameter expression is already set.
  void expression(const ExpressionPtr& expression);

  /// @returns The name of this variable.
  const std::string& name() const { return name_; }

  /// @returns The unique identifier of this parameter.
  const std::string& id() const { return id_; }

  /// @returns The unit of this parameter.
  Units unit() const { return unit_; }

  /// Sets the unit of this parameter.
  ///
  /// @param[in] unit  A valid unit.
  void unit(Units unit) { unit_ = unit; }

  /// @returns The usage state of this parameter.
  bool unused() { return unused_; }

  /// Sets the usage state for this parameter.
  ///
  /// @param[in] state  The usage state for this parameter.
  void unused(bool state) { unused_ = state; }

  double Mean() noexcept override { return expression_->Mean(); }
  double Max() noexcept override { return expression_->Max(); }
  double Min() noexcept override { return expression_->Min(); }

  /// This function is for cycle detection.
  ///
  /// @returns The connector between parameters.
  Expression* connector() { return this; }

  /// @returns The mark of this node.
  const std::string& mark() const { return mark_; }

  /// Sets the mark for this node.
  ///
  /// @param[in] label  The specific label for the node.
  void mark(const std::string& label) { mark_ = label; }

 private:
  double GetSample() noexcept override { return expression_->Sample(); }

  std::string name_;  ///< Name of this parameter or variable.
  std::string id_;  ///< Identifier of this parameter or variable.
  ExpressionPtr expression_;  ///< Expression for this parameter.
  Units unit_;  ///< Units of this parameter.
  bool unused_;  ///< Usage state.
  std::string mark_;  ///< The mark for traversal in cycle detection.
};

/// @class MissionTime
/// This is for the system mission time.
class MissionTime : public Expression {
 public:
  MissionTime();

  /// Sets the mission time.
  /// This function is expected to be used only once.
  ///
  /// @param[in] time  The mission time.
  void mission_time(double time) {
    assert(time >= 0);
    mission_time_ = time;
  }

  /// @returns The unit of the system mission time.
  Units unit() const { return unit_; }

  /// Sets the unit of this parameter.
  ///
  /// @param[in] unit  A valid unit.
  void unit(Units unit) { unit_ = unit; }

  double Mean() noexcept override { return mission_time_; }
  bool IsConstant() noexcept override { return true; }

 private:
  double GetSample() noexcept override { return mission_time_; }

  double mission_time_;  ///< The system mission time.
  Units unit_;  ///< Units of this parameter.
};

/// @class ConstantExpression
/// Indicates a constant value.
class ConstantExpression : public Expression {
 public:
  /// Constructor for numerical values.
  ///
  /// @param[in] val  Float numerical value.
  explicit ConstantExpression(double val);

  /// Constructor for numerical values.
  ///
  /// @param[in] val  Integer numerical value.
  explicit ConstantExpression(int val);

  /// Constructor for boolean values.
  ///
  /// @param[in] val  true for 1 and false for 0 value of this constant.
  explicit ConstantExpression(bool val);

  double Mean() noexcept override { return value_; }
  bool IsConstant() noexcept override { return true; }

 private:
  double GetSample() noexcept override { return value_; }
  double value_;  ///< The Constant value.
};

/// @class ExponentialExpression
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
  void Validate() override;

  double Mean() noexcept override {
    return 1 - std::exp(-(lambda_->Mean() * time_->Mean()));
  }

  double Max() noexcept override {
    return 1 - std::exp(-(lambda_->Max() * time_->Max()));
  }

  double Min() noexcept override {
    return 1 - std::exp(-(lambda_->Min() * time_->Min()));
  }

 private:
  double GetSample() noexcept override {
    return 1 - std::exp(-(lambda_->Sample() * time_->Sample()));
  }

  ExpressionPtr lambda_;  ///< Failure rate in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
};

/// @class GlmExpression
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

  void Validate() override;

  double Mean() noexcept override;
  double Max() noexcept override { return 1; }
  double Min() noexcept override { return 0; }

 private:
  double GetSample() noexcept override;

  /// Computes the value for GLM expression.
  ///
  /// @param[in] gamma  Value for probability on demand.
  /// @param[in] lambda  Value for hourly rate of failure.
  /// @param[in] mu  Value for hourly repair rate.
  /// @param[in] time  Mission time in hours.
  ///
  /// @returns Probability of failure on demand.
  double Compute(double gamma, double lambda, double mu, double time) noexcept;

  ExpressionPtr gamma_;  ///< Probability of failure on demand.
  ExpressionPtr lambda_;  ///< Failure rate in hours.
  ExpressionPtr mu_;  ///< Repair rate in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
};

/// @class WeibullExpression
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

  void Validate() override;

  double Mean() noexcept override {
    return WeibullExpression::Compute(alpha_->Mean(), beta_->Mean(),
                                      t0_->Mean(), time_->Mean());
  }

  double Max() noexcept override {
    return WeibullExpression::Compute(alpha_->Min(), beta_->Max(),
                                      t0_->Min(), time_->Max());
  }

  double Min() noexcept override {
    return WeibullExpression::Compute(alpha_->Max(), beta_->Min(),
                                      t0_->Max(), time_->Min());
  }

 private:
  double GetSample() noexcept override {
    return WeibullExpression::Compute(alpha_->Sample(), beta_->Sample(),
                                      t0_->Sample(), time_->Sample());
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

  ExpressionPtr alpha_;  ///< Scale parameter.
  ExpressionPtr beta_;  ///< Shape parameter.
  ExpressionPtr t0_;  ///< Time shift in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
};

/// @class RandomDeviate
/// Abstract base class for all deviate expressions.
/// These expressions provide quantification for uncertainty and sensitivity.
class RandomDeviate : public Expression {
 public:
  using Expression::Expression;  // Main helper constructors with arguments.

  bool IsConstant() noexcept override { return false; }
};

/// @class UniformDeviate
/// Uniform distribution.
class UniformDeviate : public RandomDeviate {
 public:
  /// Setup for uniform distribution.
  ///
  /// @param[in] min  Minimum value of the distribution.
  /// @param[in] max  Maximum value of the distribution.
  UniformDeviate(const ExpressionPtr& min, const ExpressionPtr& max);

  /// @throws InvalidArgument  The min value is more or equal to max value.
  void Validate() override;

  double Mean() noexcept override { return (min_->Mean() + max_->Mean()) / 2; }
  double Max() noexcept override { return max_->Max(); }
  double Min() noexcept override { return min_->Min(); }

 private:
  double GetSample() noexcept override;

  ExpressionPtr min_;  ///< Minimum value of the distribution.
  ExpressionPtr max_;  ///< Maximum value of the distribution.
};

/// @class NormalDeviate
/// Normal distribution.
class NormalDeviate : public RandomDeviate {
 public:
  /// Setup for normal distribution.
  ///
  /// @param[in] mean  The mean of the distribution.
  /// @param[in] sigma  The standard deviation of the distribution.
  NormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& sigma);

  /// @throws InvalidArgument  The sigma is negative or zero.
  void Validate() override;

  double Mean() noexcept override { return mean_->Mean(); }

  /// @returns ~99.9% percentile value.
  ///
  /// @warning This is only an approximation of the maximum value.
  double Max() noexcept override { return mean_->Max() + 6 * sigma_->Max(); }

  /// @returns Less than 0.1% percentile value.
  ///
  /// @warning This is only an approximation.
  double Min() noexcept override { return mean_->Min() - 6 * sigma_->Max(); }

 private:
  double GetSample() noexcept override;

  ExpressionPtr mean_;  ///< Mean value of normal distribution.
  ExpressionPtr sigma_;  ///< Standard deviation of normal distribution.
};

/// @class LogNormalDeviate
/// Log-normal distribution.
class LogNormalDeviate : public RandomDeviate {
 public:
  /// Setup for log-normal distribution.
  ///
  /// @param[in] mean  The mean of the log-normal distribution
  ///                  not the mean of underlying normal distribution,
  ///                  which is parameter mu.
  ///                  mu is the location parameter,
  ///                  sigma is the scale factor.
  ///                  E(x) = exp(mu + sigma^2 / 2)
  /// @param[in] ef  The error factor of the log-normal distribution.
  ///                EF = exp(z * sigma)
  /// @param[in] level  The confidence level.
  LogNormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& ef,
                   const ExpressionPtr& level);

  /// @throws InvalidArgument  (mean <= 0) or (ef <= 0) or invalid level
  void Validate() override;

  double Mean() noexcept override { return mean_->Mean(); }

  /// 99 percentile estimate.
  double Max() noexcept override;

  double Min() noexcept override { return 0; }

 private:
  double GetSample() noexcept override;

  /// Computes the scale parameter of the distribution.
  ///
  /// @param[in] level  The confidence level.
  /// @param[in] ef  The error factor of the log-normal distribution.
  ///
  /// @returns Scale parameter (sigma) value.
  double ComputeScale(double level, double ef) noexcept;

  /// Computes the location parameter of the distribution.
  ///
  /// @param[in] mean  The mean of the log-normal distribution.
  /// @param[in] sigma  The scale parameter of the distribution.
  ///
  /// @returns Value of location parameter (mu) value.
  double ComputeLocation(double mean, double sigma) noexcept;

  ExpressionPtr mean_;  ///< Mean value of the log-normal distribution.
  ExpressionPtr ef_;  ///< Error factor of the log-normal distribution.
  ExpressionPtr level_;  ///< Confidence level of the log-normal distribution.
};

/// @class GammaDeviate
/// Gamma distribution.
class GammaDeviate : public RandomDeviate {
 public:
  /// Setup for Gamma distribution.
  ///
  /// @param[in] k  Shape parameter of Gamma distribution.
  /// @param[in] theta  Scale parameter of Gamma distribution.
  GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta);

  /// @throws InvalidArgument  (k <= 0) or (theta <= 0)
  void Validate() override;

  double Mean() noexcept override { return k_->Mean() * theta_->Mean(); }

  /// @returns 99 percentile.
  double Max() noexcept override {
    using boost::math::gamma_q;
    return theta_->Max() *
        std::pow(gamma_q(k_->Max(), gamma_q(k_->Max(), 0) - 0.99), -1);
  }

  double Min() noexcept override { return 0; }

 private:
  double GetSample() noexcept override;

  ExpressionPtr k_;  ///< The shape parameter of the gamma distribution.
  ExpressionPtr theta_;  ///< The scale factor of the gamma distribution.
};

/// @class BetaDeviate
/// Beta distribution.
class BetaDeviate : public RandomDeviate {
 public:
  /// Setup for Beta distribution.
  ///
  /// @param[in] alpha  Alpha shape parameter of Gamma distribution.
  /// @param[in] beta  Beta shape parameter of Gamma distribution.
  BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta);

  /// @throws InvalidArgument  (alpha <= 0) or (beta <= 0)
  void Validate() override;

  double Mean() noexcept override {
    return alpha_->Mean() / (alpha_->Mean() + beta_->Mean());
  }

  /// @returns 99 percentile.
  double Max() noexcept override {
    return std::pow(boost::math::ibeta(alpha_->Max(), beta_->Max(), 0.99), -1);
  }

  double Min() noexcept override { return 0; }

 private:
  double GetSample() noexcept override;

  ExpressionPtr alpha_;  ///< The alpha shape parameter.
  ExpressionPtr beta_;  ///< The beta shape parameter.
};

/// @class Histogram
/// Histogram distribution.
class Histogram : public RandomDeviate {
 public:
  /// Histogram distribution setup.
  ///
  /// @param[in] boundaries  The upper bounds of intervals.
  /// @param[in] weights  The positive weights of intervals
  ///                     restricted by the upper boundaries.
  ///                     Therefore, the number of weights must be
  ///                     equal to the number of boundaries.
  ///
  /// @throws InvalidArgument  The boundaries container size is not equal to
  ///                          weights container size.
  ///
  /// @note This description of histogram sampling is mostly for probabilities.
  ///       Therefore, it is not flexible.
  ///       Currently, it allows sampling both boundaries and weights.
  ///       This behavior makes checking
  ///       for valid arrangement of the boundaries mandatory
  ///       for each sampling.
  ///       Moreover, the first starting point is assumed but not defined.
  ///       The starting point is assumed to be 0,
  ///       which leaves only positive values for boundaries.
  ///       This behavior is restrictive
  ///       and should be handled accordingly.
  Histogram(std::vector<ExpressionPtr> boundaries,
            std::vector<ExpressionPtr> weights);

  /// @throws InvalidArgument  The boundaries are not strictly increasing,
  ///                          or weights are negative.
  void Validate() override;

  double Mean() noexcept override;
  double Max() noexcept override { return boundaries_.back()->Max(); }
  double Min() noexcept override { return boundaries_.front()->Min(); }

 private:
  double GetSample() noexcept override;

  /// Checks if mean values of expressions are strictly increasing.
  ///
  /// @param[in] boundaries  The upper bounds of intervals.
  ///
  /// @throws InvalidArgument  The mean values are not strictly increasing.
  void CheckBoundaries(const std::vector<ExpressionPtr>& boundaries);

  /// Checks if mean values of boundaries are non-negative.
  ///
  /// @param[in] weights  The positive weights of intervals restricted by
  ///                     the upper boundaries.
  ///
  /// @throws InvalidArgument  The mean values are negative.
  void CheckWeights(const std::vector<ExpressionPtr>& weights);

  /// Upper boundaries of the histogram.
  std::vector<ExpressionPtr> boundaries_;

  /// Weights of intervals described by boundaries.
  std::vector<ExpressionPtr> weights_;
};

/// @class Neg
/// This class for negation of numerical value or another expression.
class Neg : public Expression {
 public:
  /// Construct a new expression
  /// that negates a given argument expression.
  ///
  /// @param[in] expression  The expression to be negated.
  explicit Neg(const ExpressionPtr& expression);

  double Mean() noexcept override { return -expression_->Mean(); }
  double Max() noexcept override { return -expression_->Min(); }
  double Min() noexcept override { return -expression_->Max(); }

 private:
  double GetSample() noexcept override { return -expression_->Sample(); }

  ExpressionPtr expression_;  ///< Expression that is used for negation.
};

/// @class Add
/// This expression adds all the given expressions' values.
class Add : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.

  double Mean() noexcept override;
  double Max() noexcept override;
  double Min() noexcept override;

 private:
  double GetSample() noexcept override;
};

/// @class Sub
/// This expression performs subtraction operation.
/// First expression minus the rest of the given expressions' values.
class Sub : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.

  double Mean() noexcept override;
  double Max() noexcept override;
  double Min() noexcept override;

 private:
  double GetSample() noexcept override;
};

/// @class Mul
/// This expression performs multiplication operation.
class Mul : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.

  double Mean() noexcept override;

  /// Finds maximum product
  /// from the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Maximum possible value of the product.
  double Max() noexcept override { return Mul::GetExtremum(/*max=*/true); }

  /// Finds minimum product
  /// from the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Minimum possible value of the product.
  double Min() noexcept override { return Mul::GetExtremum(/*max=*/false); }

 private:
  double GetSample() noexcept override;

  /// @param[in] maximum  Flag to return maximum value.
  ///
  /// @returns One of extremums.
  double GetExtremum(bool maximum) noexcept;
};

/// @class Div
/// This expression performs division operation.
/// The expression divides the first given argument by
/// the rest of argument expressions.
class Div : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.

  /// @throws InvalidArgument  Division by 0.
  void Validate() override;

  double Mean() noexcept override;

  /// Finds maximum results of division
  /// of the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Maximum value for division of arguments.
  double Max() noexcept override { return Div::GetExtremum(/*max=*/true); }

  /// Finds minimum results of division
  /// of the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Minimum value for division of arguments.
  double Min() noexcept override { return Div::GetExtremum(/*max=*/false); }

 private:
  double GetSample() noexcept override;

  /// @param[in] maximum  Flag to return maximum value.
  ///
  /// @returns One of extremums.
  double GetExtremum(bool maximum) noexcept;
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
