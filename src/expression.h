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

/// @file expression.h
/// Expressions that describe basic events.

#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/math/special_functions/beta.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/gamma.hpp>

#include "element.h"

namespace scram {

class Parameter;  // This is for cycle detection through expressions.

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
  typedef std::shared_ptr<Expression> ExpressionPtr;

  /// Constructor for use by derived classes
  /// to register their arguments.
  ///
  /// @param[in] args Arguments of this expression.
  explicit Expression(const std::vector<ExpressionPtr>& args);

  Expression(const Expression&) = delete;
  Expression& operator=(const Expression&) = delete;

  virtual ~Expression() {}

  /// Validates the expression.
  /// This late validation is due to parameters that are defined late.
  ///
  /// @throws InvalidArgument The arguments are invalid for setup.
  virtual void Validate() {}

  /// @returns The mean value of this expression.
  virtual double Mean() noexcept = 0;

  /// @returns A sampled value of this expression.
  virtual double Sample() noexcept = 0;

  /// This routine resets the sampling to get a new values.
  /// All the arguments are called to reset themselves.
  /// If this expression was not sampled,
  /// its arguments are not going to get any calls.
  virtual void Reset() noexcept;

  /// Determines if the value of the expression varies.
  /// The default logic is to check arguments with uncertainties for sampling.
  /// Derivied expression classes must decide
  /// if they don't have arguments,
  /// or if they are random deviates.
  ///
  /// @returns true if the expression's value does not need sampling.
  /// @returns false if the expression's value has uncertainties.
  ///
  /// @warning Improper registration of arguments
  ///          may yeild silent failure.
  virtual bool IsConstant() noexcept;

  /// @returns Maximum value of this expression.
  inline virtual double Max() noexcept { return Mean(); }

  /// @returns Minimum value of this expression.
  inline virtual double Min() noexcept { return Mean(); }

  /// @returns Parameters as nodes.
  inline const std::vector<Parameter*>& nodes() {
    if (gather_) Expression::GatherNodesAndConnectors();
    return nodes_;
  }

  /// @returns Non-Parameter Expressions as connectors.
  inline const std::vector<Expression*>& connectors() {
    if (gather_) Expression::GatherNodesAndConnectors();
    return connectors_;
  }

 protected:
  /// Constructor for use by derived classes
  /// that do not have or need any arguments
  /// or can't register arguments on their construction.
  ///
  /// @note Derived classes must register their arguments
  ///       for validation and other common functionality.
  Expression() {}

  bool sampled_ = false;  ///< Indication if the expression is already sampled.
  double sampled_value_ = 0;  ///< The sampled value.
  std::vector<ExpressionPtr> args_;  ///< Expressions arguments.

 private:
  /// Gathers nodes and connectors from arguments of the expression.
  void GatherNodesAndConnectors();

  std::vector<Parameter*> nodes_;  ///< Parameters as nodes.
  std::vector<Expression*> connectors_;  ///< Expressions as connectors.
  bool gather_ = true;  ///< A flag to gather nodes and connectors.
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
  /// @param[in] name The name of this variable (Case sensitive).
  /// @param[in] base_path The series of containers to get this parameter.
  /// @param[in] is_public Whether or not the parameter is public.
  explicit Parameter(const std::string& name,
                     const std::string& base_path = "",
                     bool is_public = true);

  /// Sets the expression of this parameter.
  ///
  /// @param[in] expression The expression to describe this parameter.
  inline void expression(const ExpressionPtr& expression) {
    expression_ = expression;
    Expression::args_.clear();
    Expression::args_.push_back(expression);
  }

  /// @returns The name of this variable.
  inline const std::string& name() const { return name_; }

  /// @returns The unique identifier  of this parameter.
  inline const std::string& id() const { return id_; }

  /// @returns The unit of this parameter.
  inline const Units& unit() const { return unit_; }

  /// Sets the unit of this parameter.
  ///
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  /// @returns The usage state of this parameter.
  inline bool unused() { return unused_; }

  /// Sets the usage state for this parameter.
  ///
  /// @param[in] state The usage state for this parameter.
  inline void unused(bool state) { unused_ = state; }

  inline double Mean() noexcept { return expression_->Mean(); }

  inline double Sample() noexcept {
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      Expression::sampled_value_ = expression_->Sample();
    }
    return Expression::sampled_value_;
  }

  inline double Max() noexcept { return expression_->Max(); }
  inline double Min() noexcept { return expression_->Min(); }

  /// This function is for cycle detection.
  ///
  /// @returns The connector between parameters.
  inline Expression* connector() { return this; }

  /// @returns The mark of this node.
  inline const std::string& mark() const { return mark_; }

  /// Sets the mark for this node.
  ///
  /// @param[in] label The specific label for the node.
  inline void mark(const std::string& label) { mark_ = label; }

 private:
  std::string name_;  ///< Name of this parameter or variable.
  std::string id_;  ///< Identifier of this parameter or variable.
  Units unit_;  ///< Units of this parameter.
  ExpressionPtr expression_;  ///< Expression for this parameter.
  std::string mark_;  ///< The mark for traversal in cycle detection.
  bool unused_;  ///< Usage state.
};

/// @class MissionTime
/// This is for the system mission time.
class MissionTime : public Expression {
 public:
  MissionTime() : mission_time_(-1), unit_(kHours) {}

  /// Sets the mission time.
  /// This function is expected to be used only once.
  ///
  /// @param[in] time The mission time.
  inline void mission_time(double time) {
    assert(time >= 0);
    mission_time_ = time;
  }

  /// @returns The unit of the system mission time.
  inline const Units& unit() const { return unit_; }

  /// Sets the unit of this parameter.
  ///
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  inline double Mean() noexcept { return mission_time_; }
  inline double Sample() noexcept { return mission_time_; }
  inline bool IsConstant() noexcept { return true; }

 private:
  double mission_time_;  ///< The constant's value.
  Units unit_;  ///< Units of this parameter.
};

/// @class ConstantExpression
/// Indicates a constant value.
class ConstantExpression : public Expression {
 public:
  /// Constructor for float values.
  ///
  /// @param[in] val Float numerical value.
  explicit ConstantExpression(double val) : value_(val) {}

  /// Constructor for integer values.
  ///
  /// @param[in] val Integer numerical value.
  explicit ConstantExpression(int val) : value_(val) {}

  /// Constructor for boolean values.
  ///
  /// @param[in] val true for 1 and false for 0 value of this constant.
  explicit ConstantExpression(bool val) : value_(val ? 1 : 0) {}

  inline double Mean() noexcept { return value_; }
  inline double Sample() noexcept { return value_; }
  inline bool IsConstant() noexcept { return true; }

 private:
  double value_;  ///< The constant's value.
};

/// @class ExponentialExpression
/// Negative exponential distribution
/// with hourly failure rate and time.
class ExponentialExpression : public Expression {
 public:
  /// Constructor for exponential expression with two arguments.
  ///
  /// @param[in] lambda Hourly rate of failure.
  /// @param[in] t Mission time in hours.
  ExponentialExpression(const ExpressionPtr& lambda, const ExpressionPtr& t)
      : Expression::Expression({lambda, t}),
        lambda_(lambda),
        time_(t) {}

  /// @throws InvalidArgument The failure rate or time is negative.
  void Validate();

  inline double Mean() noexcept {
    return 1 - std::exp(-(lambda_->Mean() * time_->Mean()));
  }

  double Sample() noexcept;

  inline double Max() noexcept {
    return 1 - std::exp(-(lambda_->Max() * time_->Max()));
  }

  inline double Min() noexcept {
    return 1 - std::exp(-(lambda_->Min() * time_->Min()));
  }

 private:
  ExpressionPtr lambda_;  ///< Failure rate in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
};

/// @class GlmExpression
/// Exponential with probability of failure on demand,
/// hourly failure rate, hourly repairing rate, and time.
class GlmExpression : public Expression {
 public:
  /// Constructor for GLM or exponential expression with four arguments.
  ///
  /// @param[in] gamma Probability of failure on demand.
  /// @param[in] lambda Hourly rate of failure.
  /// @param[in] mu Hourly repairing rate.
  /// @param[in] t Mission time in hours.
  GlmExpression(const ExpressionPtr& gamma, const ExpressionPtr& lambda,
                const ExpressionPtr& mu, const ExpressionPtr& t)
      : Expression::Expression({gamma, lambda, mu, t}),
        gamma_(gamma),
        lambda_(lambda),
        mu_(mu),
        time_(t) {}

  void Validate();

  inline double Mean() noexcept {
    double r = lambda_->Mean() + mu_->Mean();
    return (lambda_->Mean() - (lambda_->Mean() - gamma_->Mean() * r) *
            std::exp(-r * time_->Mean())) / r;
  }

  double Sample() noexcept;

  inline double Max() noexcept {
    /// @todo Find the maximum case.
    return 1;
  }

  inline double Min() noexcept {
    /// @todo Find the minimum case.
    return 0;
  }

 private:
  ExpressionPtr gamma_;  ///< Probabilty of failure on demand.
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
  /// @param[in] alpha Scale parameter.
  /// @param[in] beta Shape parameter.
  /// @param[in] t0 Time shift.
  /// @param[in] time Mission time.
  WeibullExpression(const ExpressionPtr& alpha, const ExpressionPtr& beta,
                    const ExpressionPtr& t0, const ExpressionPtr& time)
      : Expression::Expression({alpha, beta, t0, time}),
        alpha_(alpha),
        beta_(beta),
        t0_(t0),
        time_(time) {}

  void Validate();

  inline double Mean() noexcept {
    return 1 - std::exp(-std::pow((time_->Mean() - t0_->Mean()) /
                                  alpha_->Mean(), beta_->Mean()));
  }

  double Sample() noexcept;

  inline double Max() noexcept {
    return 1 - std::exp(-std::pow((time_->Max() - t0_->Min()) /
                                  alpha_->Min(), beta_->Max()));
  }

  inline double Min() noexcept {
    return 1 - std::exp(-std::pow((time_->Min() - t0_->Max()) /
                                  alpha_->Max(), beta_->Min()));
  }

 private:
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
  virtual ~RandomDeviate() = 0;  ///< Make it abstract.

  inline bool IsConstant() noexcept { return false; }
};

/// @class UniformDeviate
/// Uniform distribution.
class UniformDeviate : public RandomDeviate {
 public:
  /// Setup for uniform distribution.
  ///
  /// @param[in] min Minimum value of the distribution.
  /// @param[in] max Maximum value of the distribution.
  UniformDeviate(const ExpressionPtr& min, const ExpressionPtr& max)
      : RandomDeviate::RandomDeviate({min, max}),
        min_(min),
        max_(max) {}

  /// @throws InvalidArgument The min value is more or equal to max value.
  void Validate();

  inline double Mean() noexcept { return (min_->Mean() + max_->Mean()) / 2; }

  double Sample() noexcept;

  inline double Max() noexcept { return max_->Max(); }
  inline double Min() noexcept { return min_->Min(); }

 private:
  ExpressionPtr min_;  ///< Minimum value of the distribution.
  ExpressionPtr max_;  ///< Maximum value of the distribution.
};

/// @class NormalDeviate
/// Normal distribution.
class NormalDeviate : public RandomDeviate {
 public:
  /// Setup for normal distribution.
  ///
  /// @param[in] mean The mean of the distribution.
  /// @param[in] sigma The standard deviation of the distribution.
  NormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& sigma)
      : RandomDeviate::RandomDeviate({mean, sigma}),
        mean_(mean),
        sigma_(sigma) {}

  /// @throws InvalidArgument The sigma is negative or zero.
  void Validate();

  inline double Mean() noexcept { return mean_->Mean(); }

  double Sample() noexcept;

  /// @returns ~99.9% percentile value.
  ///
  /// @warning This is only an approximation of the maximum value.
  inline double Max() noexcept { return mean_->Max() + 6 * sigma_->Max(); }

  /// @returns Less than 0.1% percentile value.
  ///
  /// @warning This is only an approximation.
  inline double Min() noexcept { return mean_->Min() - 6 * sigma_->Max(); }

 private:
  ExpressionPtr mean_;  ///< Mean value of normal distribution.
  ExpressionPtr sigma_;  ///< Standard deviation of normal distribution.
};

/// @class LogNormalDeviate
/// Log-normal distribution.
class LogNormalDeviate : public RandomDeviate {
 public:
  /// Setup for log-normal distribution.
  ///
  /// @param[in] mean The mean of the log-normal distribution
  ///                 not the mean of underlying normal distribution,
  ///                 which is parameter mu.
  ///                 mu is the location parameter,
  ///                 sigma is the scale factor.
  ///                 E(x) = exp(mu + sigma^2 / 2)
  /// @param[in] ef The error factor of the log-normal distribution.
  ///               EF = exp(z * sigma)
  /// @param[in] level The confidence level.
  LogNormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& ef,
                   const ExpressionPtr& level)
      : RandomDeviate::RandomDeviate({mean, ef, level}),
        mean_(mean),
        ef_(ef),
        level_(level) {}

  /// @throws InvalidArgument (mean <= 0) or (ef <= 0) or invalid level
  void Validate();

  inline double Mean() noexcept { return mean_->Mean(); }

  double Sample() noexcept;

  /// 99 percentile estimate.
  inline double Max() noexcept {
    double p = level_->Mean() + (1 - level_->Mean()) / 2;
    double z = std::sqrt(2) * boost::math::erfc_inv(2 * p);
    z = std::abs(z);
    double sigma = std::log(ef_->Mean()) / z;
    double mu = std::log(mean_->Max()) - std::pow(sigma, 2) / 2;
    return std::exp(std::sqrt(2) * std::pow(boost::math::erfc(1 / 50), -1) *
                    sigma + mu);
  }

  inline double Min() noexcept { return 0; }

 private:
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
  /// @param[in] k Shape parameter of Gamma distribution.
  /// @param[in] theta Scale parameter of Gamma distribution.
  GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta)
      : RandomDeviate::RandomDeviate({k, theta}),
        k_(k),
        theta_(theta) {}

  /// @throws InvalidArgument (k <= 0) or (theta <= 0)
  void Validate();

  inline double Mean() noexcept { return k_->Mean() * theta_->Mean(); }

  double Sample() noexcept;

  inline double Max() noexcept {
    return theta_->Max() *
        std::pow(boost::math::gamma_q(k_->Max(),
                                      boost::math::gamma_q(k_->Max(), 0)
                                      - 0.99),
                 -1);
  }

  inline double Min() noexcept { return 0; }

 private:
  ExpressionPtr k_;  ///< The shape parameter of the gamma distribution.
  ExpressionPtr theta_;  ///< The scale factor of the gamma distribution.
};

/// @class BetaDeviate
/// Beta distribution.
class BetaDeviate : public RandomDeviate {
 public:
  /// Setup for Beta distribution.
  ///
  /// @param[in] alpha Alpha shape parameter of Gamma distribution.
  /// @param[in] beta Beta shape parameter of Gamma distribution.
  BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta)
      : RandomDeviate::RandomDeviate({alpha, beta}),
        alpha_(alpha),
        beta_(beta) {}

  /// @throws InvalidArgument (alpha <= 0) or (beta <= 0)
  void Validate();

  inline double Mean() noexcept {
    return alpha_->Mean() / (alpha_->Mean() + beta_->Mean());
  }

  double Sample() noexcept;

  /// 99 percentile estimate.
  inline double Max() noexcept {
    return std::pow(boost::math::ibeta(alpha_->Max(), beta_->Max(), 0.99), -1);
  }

  inline double Min() noexcept { return 0; }

 private:
  ExpressionPtr alpha_;  ///< The alpha shape parameter.
  ExpressionPtr beta_;  ///< The beta shape parameter.
};

/// @class Histogram
/// Histogram distribution.
class Histogram : public RandomDeviate {
 public:
  /// Histogram distribution setup.
  ///
  /// @param[in] boundaries The upper bounds of intervals.
  /// @param[in] weights The positive weights of intervals
  ///                    restricted by the upper boundaries.
  ///                    Therefore, the number of weights must be
  ///                    equal to the number of boundaries.
  ///
  /// @throws InvalidArgument The boundaries container size is not equal to
  ///                         weights container size.
  ///
  /// @note This description of histogram sampling is for probabilities mostly.
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
  Histogram(const std::vector<ExpressionPtr>& boundaries,
            const std::vector<ExpressionPtr>& weights);

  /// @throws InvalidArgument The boundaries are not strictly increasing,
  ///                         or weights are negative.
  void Validate();

  double Mean() noexcept;
  double Sample() noexcept;

  inline double Max() noexcept { return boundaries_.back()->Max(); }
  inline double Min() noexcept { return boundaries_.front()->Min(); }

 private:
  /// Checks if mean values of expressions are strictly increasing.
  ///
  /// @param[in] boundaries The upper bounds of intervals.
  ///
  /// @throws InvalidArgument The mean values are not strictly increasing.
  void CheckBoundaries(const std::vector<ExpressionPtr>& boundaries);

  /// Checks if mean values of boundaries are non-negative.
  ///
  /// @param[in] weights The positive weights of intervals restricted by
  ///                    the upper boundaries.
  ///
  /// @throws InvalidArgument The mean values are negative.
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
  /// @param[in] expression The expression to be negated.
  explicit Neg(const ExpressionPtr& expression)
      : Expression::Expression({expression}),
        expression_(expression) {}

  inline double Mean() noexcept { return -expression_->Mean(); }

  inline double Sample() noexcept {
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      Expression::sampled_value_ = -expression_->Sample();
    }
    return Expression::sampled_value_;
  }

  inline double Max() noexcept { return -expression_->Min(); }
  inline double Min() noexcept { return -expression_->Max(); }

 private:
  ExpressionPtr expression_;  ///< Expression that is used for negation.
};

/// @class Add
/// This expression adds all the given expressions' values.
class Add : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.
  Add() = delete;

  inline double Mean() noexcept {
    assert(!args_.empty());
    double mean = 0;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      mean += (*it)->Mean();
    }
    return mean;
  }

  inline double Sample() noexcept {
    assert(!args_.empty());
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      Expression::sampled_value_ = 0;
      std::vector<ExpressionPtr>::iterator it;
      for (it = args_.begin(); it != args_.end(); ++it) {
        Expression::sampled_value_ += (*it)->Sample();
      }
    }
    return Expression::sampled_value_;
  }

  inline double Max() noexcept {
    assert(!args_.empty());
    double max = 0;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      max += (*it)->Max();
    }
    return max;
  }

  inline double Min() noexcept {
    assert(!args_.empty());
    double min = 0;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      min += (*it)->Min();
    }
    return min;
  }
};

/// @class Sub
/// This expression performs subtraction operation.
/// First expression minus the rest of the given expressions' values.
class Sub : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.
  Sub() = delete;

  inline double Mean() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double mean = (*it)->Mean();
    for (++it; it != args_.end(); ++it) {
      mean -= (*it)->Mean();
    }
    return mean;
  }

  inline double Sample() noexcept {
    assert(!args_.empty());
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      std::vector<ExpressionPtr>::iterator it = args_.begin();
      Expression::sampled_value_ = (*it)->Sample();
      for (++it; it != args_.end(); ++it) {
        Expression::sampled_value_ -= (*it)->Sample();
      }
    }
    return Expression::sampled_value_;
  }

  inline double Max() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double max = (*it)->Max();
    for (++it; it != args_.end(); ++it) {
      max -= (*it)->Min();
    }
    return max;
  }

  inline double Min() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double min = (*it)->Min();
    for (++it; it != args_.end(); ++it) {
      min -= (*it)->Max();
    }
    return min;
  }
};

/// @class Mul
/// This expression performs multiplication operation.
class Mul : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.
  Mul() = delete;

  inline double Mean() noexcept {
    assert(!args_.empty());
    double mean = 1;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      mean *= (*it)->Mean();
    }
    return mean;
  }

  inline double Sample() noexcept {
    assert(!args_.empty());
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      Expression::sampled_value_ = 1;
      std::vector<ExpressionPtr>::iterator it;
      for (it = args_.begin(); it != args_.end(); ++it) {
        Expression::sampled_value_ *= (*it)->Sample();
      }
    }
    return Expression::sampled_value_;
  }

  /// Finds maximum product
  /// from the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Maximum possible value of the product.
  inline double Max() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double max = (*it)->Max();  // Maximum possible product.
    double min = (*it)->Min();  // Minimum possible product.
    for (++it; it != args_.end(); ++it) {
      double mult_max = (*it)->Max();
      double mult_min = (*it)->Min();
      double max_max = max * mult_max;
      double max_min = max * mult_min;
      double min_max = min * mult_max;
      double min_min = min * mult_min;
      max = std::max(std::max(max_max, max_min), std::max(min_max, min_min));
      min = std::min(std::min(max_max, max_min), std::min(min_max, min_min));
    }
    return max;
  }

  /// Finds minimum product
  /// from the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Minimum possible value of the product.
  inline double Min() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double max = (*it)->Max();  // Maximum possible product.
    double min = (*it)->Min();  // Minimum possible product.
    for (++it; it != args_.end(); ++it) {
      double mult_max = (*it)->Max();
      double mult_min = (*it)->Min();
      double max_max = max * mult_max;
      double max_min = max * mult_min;
      double min_max = min * mult_max;
      double min_min = min * mult_min;
      max = std::max(std::max(max_max, max_min), std::max(min_max, min_min));
      min = std::min(std::min(max_max, max_min), std::min(min_max, min_min));
    }
    return min;
  }
};

/// @class Div
/// This expression performs division operation.
/// The expression divides the first given argument by
/// the rest of argument expressions.
class Div : public Expression {
 public:
  using Expression::Expression;  // Base class constructors with arguments.
  Div() = delete;

  inline double Mean() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double mean = (*it)->Mean();
    for (++it; it != args_.end(); ++it) {
      mean /= (*it)->Mean();
    }
    return mean;
  }

  inline double Sample() noexcept {
    assert(!args_.empty());
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      std::vector<ExpressionPtr>::iterator it = args_.begin();
      Expression::sampled_value_ = (*it)->Sample();
      for (++it; it != args_.end(); ++it) {
        Expression::sampled_value_ /= (*it)->Sample();
      }
    }
    return Expression::sampled_value_;
  }

  /// Finds maximum results of division
  /// of the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Maximum value for division of arguments.
  inline double Max() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double max = (*it)->Max();  // Maximum possible result.
    double min = (*it)->Min();  // Minimum possible result.
    for (++it; it != args_.end(); ++it) {
      double mult_max = (*it)->Max();
      double mult_min = (*it)->Min();
      double max_max = max / mult_max;
      double max_min = max / mult_min;
      double min_max = min / mult_max;
      double min_min = min / mult_min;
      max = std::max(std::max(max_max, max_min), std::max(min_max, min_min));
      min = std::min(std::min(max_max, max_min), std::min(min_max, min_min));
    }
    return max;
  }

  /// Finds minimum results of division
  /// of the given arguments' minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  ///
  /// @returns Minimum value for division of arguments.
  inline double Min() noexcept {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double max = (*it)->Max();  // Maximum possible result.
    double min = (*it)->Min();  // Minimum possible result.
    for (++it; it != args_.end(); ++it) {
      double mult_max = (*it)->Max();
      double mult_min = (*it)->Min();
      double max_max = max / mult_max;
      double max_min = max / mult_min;
      double min_max = min / mult_max;
      double min_min = min / mult_min;
      max = std::max(std::max(max_max, max_min), std::max(min_max, min_min));
      min = std::min(std::min(max_max, max_min), std::min(min_max, min_min));
    }
    return min;
  }
};

}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
