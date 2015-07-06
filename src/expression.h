/// @file expression.h
/// Expressions that describe basic events.
#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

#include <boost/math/special_functions/beta.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/shared_ptr.hpp>

#include "element.h"

namespace scram {

class Parameter;  // This is for cycle detection through expressions.

/// @class Expression
/// Abstract base class for all sorts of expressions to describe events.
/// This class also acts like a connector for parameter nodes and may
/// create cycles. Expressions are not expected to be shared except for
/// parameters. In addition, expressions are not expected to be changed
/// after validation phases.
class Expression {
 public:
  typedef boost::shared_ptr<Expression> ExpressionPtr;

  Expression() : sampled_(false), sampled_value_(0), gather_(true) {}

  virtual ~Expression() {}

  /// Validates the expression.
  /// This late validation is due to parameters that are defined late.
  /// @throws InvalidArgument if arguments are invalid for setup.
  virtual void Validate() {}

  /// @returns The mean value of this expression.
  virtual double Mean() = 0;

  /// @returns A sampled value of this expression.
  virtual double Sample() = 0;

  /// This routine resets the sampling to get a new values.
  virtual inline void Reset() { sampled_ = false; }

  /// Indication of constant expression.
  virtual inline bool IsConstant() { return false; }

  /// Maximum value of this expression. This indication is for sampling cases.
  virtual inline double Max() { return Mean(); }

  /// Minimum value of this expression. This indication is for sampling cases.
  virtual inline double Min() { return Mean(); }

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
  bool sampled_;  ///< Indication if the expression is already sampled.
  double sampled_value_;  ///< The sampled value.
  std::vector<ExpressionPtr> args_;  ///< Expressions arguments.

 private:
  /// Gathers nodes and connectors from arguments of the expression.
  void GatherNodesAndConnectors();

  std::vector<Parameter*> nodes_;  ///< Parameters as nodes.
  std::vector<Expression*> connectors_;  ///< Expressions as connectors.
  bool gather_;  ///< A flag to gather nodes and connectors.
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
/// This class provides a representation of a variable in basic event
/// description. It is both expression and element description.
class Parameter : public Expression, public Element, public Role {
 public:
  /// Sets the expression of this basic event.
  /// @param[in] name The name of this variable (Case sensitive).
  /// @param[in] base_path The series of containers to get this parameter.
  /// @param[in] is_public Whether or not the parameter is public.
  explicit Parameter(const std::string& name,
                     const std::string& base_path = "",
                     bool is_public = true);

  /// Sets the expression of this parameter.
  /// @param[in] expression The expression to describe this parameter.
  inline void expression(const ExpressionPtr& expression) {
    expression_ = expression;
    Expression::args_.push_back(expression);
  }

  /// @returns The name of this variable.
  inline const std::string& name() const { return name_; }

  /// @returns The unique identifier  of this parameter.
  inline const std::string& id() const { return id_; }

  /// @returns The unit of this parameter.
  inline const Units& unit() const { return unit_; }

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  /// @returns The usage state of this parameter.
  inline bool unused() { return unused_; }

  /// Sets the usage state for this parameter.
  /// @param[in] state The usage state for this parameter.
  inline void unused(bool state) { unused_ = state; }

  inline double Mean() { return expression_->Mean(); }

  inline double Sample() {
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      Expression::sampled_value_ = expression_->Sample();
    }
    return Expression::sampled_value_;
  }

  inline void Reset() {
    Expression::sampled_ = false;
    expression_->Reset();
  }

  inline bool IsConstant() { return expression_->IsConstant(); }
  inline double Max() { return expression_->Max(); }
  inline double Min() { return expression_->Min(); }

  /// This function is for cycle detection.
  /// @returns The connector between parameters.
  inline Expression* connector() { return this; }

  /// @returns The mark of this node.
  inline const std::string& mark() const { return mark_; }

  /// Sets the mark for this node.
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

  /// Sets the mission time only once.
  /// @param[in] time The mission time.
  void mission_time(double time) {
    assert(time >= 0);
    mission_time_ = time;
  }

  /// @returns The unit of the system mission time.
  inline const Units& unit() const { return unit_; }

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  inline double Mean() { return mission_time_; }
  inline double Sample() { return mission_time_; }
  inline bool IsConstant() { return true; }

 private:
  double mission_time_;  ///< The constant's value.
  Units unit_;  ///< Units of this parameter.
};

/// @class ConstantExpression
/// Indicates a constant value.
class ConstantExpression : public Expression {
 public:
  /// Constructor for float values.
  /// @param[in] val Float numerical value.
  explicit ConstantExpression(double val) : value_(val) {}

  /// Constructor for integer values.
  /// @param[in] val Integer numerical value.
  explicit ConstantExpression(int val) : value_(val) {}

  /// Constructor for boolean values.
  /// @param[in] val true for 1 and false for 0 value of this constant.
  explicit ConstantExpression(bool val) : value_(val ? 1 : 0) {}

  inline double Mean() { return value_; }
  inline double Sample() { return value_; }
  inline bool IsConstant() { return true; }

 private:
  double value_;  ///< The constant's value.
};

/// @class ExponentialExpression
/// Negative exponential distribution with hourly failure rate and time.
class ExponentialExpression : public Expression {
 public:
  /// Constructor for exponential expression with two arguments.
  /// @param[in] lambda Hourly rate of failure.
  /// @param[in] t Mission time in hours.
  ExponentialExpression(const ExpressionPtr& lambda, const ExpressionPtr& t)
      : lambda_(lambda),
        time_(t) {
    Expression::args_.push_back(lambda);
    Expression::args_.push_back(t);
  }

  /// @throws InvalidArgument if failure rate or time is negative.
  void Validate();

  inline double Mean() {
    return 1 - std::exp(-(lambda_->Mean() * time_->Mean()));
  }

  /// Samples the underlying distributions.
  /// @returns A sampled value.
  /// @throws InvalidArgument if sampled failure rate or time is negative.
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    lambda_->Reset();
    time_->Reset();
  }

  inline bool IsConstant() {
    return lambda_->IsConstant() && time_->IsConstant();
  }

  inline double Max() {
    return 1 - std::exp(-(lambda_->Max() * time_->Max()));
  }

  inline double Min() {
    return 1 - std::exp(-(lambda_->Min() * time_->Min()));
  }

 private:
  ExpressionPtr lambda_;  ///< Failure rate in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
};

/// @class GlmExpression
/// Exponential with probability of failure on demand, hourly failure rate,
/// hourly repairing rate, and time.
class GlmExpression : public Expression {
 public:
  /// Constructor for GLM or exponential expression with four arguments.
  /// @param[in] gamma Probability of failure on demand.
  /// @param[in] lambda Hourly rate of failure.
  /// @param[in] mu Hourly repairing rate.
  /// @param[in] t Mission time in hours.
  GlmExpression(const ExpressionPtr& gamma, const ExpressionPtr& lambda,
                const ExpressionPtr& mu, const ExpressionPtr& t)
      : gamma_(gamma),
        lambda_(lambda),
        mu_(mu),
        time_(t) {
    Expression::args_.push_back(gamma);
    Expression::args_.push_back(lambda);
    Expression::args_.push_back(mu);
    Expression::args_.push_back(t);
  }

  void Validate();

  inline double Mean() {
    double r = lambda_->Mean() + mu_->Mean();
    return (lambda_->Mean() - (lambda_->Mean() - gamma_->Mean() * r) *
            std::exp(-r * time_->Mean())) / r;
  }

  /// Samples the underlying distributions.
  /// @returns A sampled value.
  /// @throws InvalidArgument if sampled values are invalid.
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    gamma_->Reset();
    lambda_->Reset();
    time_->Reset();
    mu_->Reset();
  }

  inline bool IsConstant() {
    return gamma_->IsConstant() && lambda_->IsConstant() &&
        time_->IsConstant() && mu_->IsConstant();
  }

  inline double Max() {
    /// @todo Find the maximum case.
    return 1;
  }

  inline double Min() {
    /// @todo Find the minimum case.
    return 0;
  }

 private:
  ExpressionPtr gamma_;  ///< Failure rate in hours.
  ExpressionPtr lambda_;  ///< Failure rate in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
  ExpressionPtr mu_;  ///< Mission time in hours.
};

/// @class WeibullExpression
/// Weibull distribution with scale, shape, time shift, and time.
class WeibullExpression : public Expression {
 public:
  /// Constructor for Weibull distribution.
  /// @param[in] alpha Scale parameter.
  /// @param[in] beta Shape parameter.
  /// @param[in] t0 Time shift.
  /// @param[in] time Mission time.
  WeibullExpression(const ExpressionPtr& alpha, const ExpressionPtr& beta,
                    const ExpressionPtr& t0, const ExpressionPtr& time)
      : alpha_(alpha),
        beta_(beta),
        t0_(t0),
        time_(time) {
    Expression::args_.push_back(alpha);
    Expression::args_.push_back(beta);
    Expression::args_.push_back(t0);
    Expression::args_.push_back(time);
  }

  void Validate();

  inline double Mean() {
    return 1 - std::exp(-std::pow((time_->Mean() - t0_->Mean()) /
                                  alpha_->Mean(), beta_->Mean()));
  }

  /// Samples the underlying distributions.
  /// @returns A sampled value.
  /// @throws InvalidArgument if sampled values are invalid.
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    alpha_->Reset();
    beta_->Reset();
    t0_->Reset();
    time_->Reset();
  }

  inline bool IsConstant() {
    return alpha_->IsConstant() && beta_->IsConstant() && t0_->IsConstant() &&
        time_->IsConstant();
  }

  inline double Max() {
    return 1 - std::exp(-std::pow((time_->Max() - t0_->Min()) /
                                  alpha_->Min(), beta_->Max()));
  }

  inline double Min() {
    return 1 - std::exp(-std::pow((time_->Min() - t0_->Max()) /
                                  alpha_->Max(), beta_->Min()));
  }

 private:
  ExpressionPtr alpha_;  ///< Scale parameter.
  ExpressionPtr beta_;  ///< Shape parameter.
  ExpressionPtr t0_;  ///< Time shift in hours.
  ExpressionPtr time_;  ///< Mission time in hours.
};

/// @class UniformDeviate
/// Uniform distribution.
class UniformDeviate : public Expression {
 public:
  /// Setup for uniform distribution.
  /// @param[in] min Minimum value of the distribution.
  /// @param[in] max Maximum value of the distribution.
  UniformDeviate(const ExpressionPtr& min, const ExpressionPtr& max)
      : min_(min),
        max_(max) {
    Expression::args_.push_back(min);
    Expression::args_.push_back(max);
  }

  /// @throws InvalidArgument if min value is more or equal to max value.
  void Validate();

  inline double Mean() { return (min_->Mean() + max_->Mean()) / 2; }

  /// Samples the underlying distributions and uniform distribution.
  /// @returns A sampled value.
  /// @throws InvalidArgument if min value is more or equal to max value.
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    min_->Reset();
    max_->Reset();
  }

  inline double Max() { return max_->Max(); }
  inline double Min() { return min_->Min(); }

 private:
  ExpressionPtr min_;  ///< Minimum value of the distribution.
  ExpressionPtr max_;  ///< Maximum value of the distribution.
};

/// @class NormalDeviate
/// Normal distribution.
class NormalDeviate : public Expression {
 public:
  /// Setup for normal distribution with validity check for arguments.
  /// @param[in] mean The mean of the distribution.
  /// @param[in] sigma The standard deviation of the distribution.
  NormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& sigma)
      : mean_(mean),
        sigma_(sigma) {
    Expression::args_.push_back(mean);
    Expression::args_.push_back(sigma);
  }

  /// @throws InvalidArgument if sigma is negative or zero.
  void Validate();

  inline double Mean() { return mean_->Mean(); }

  /// Samples the normal distribution.
  /// @returns A sampled value.
  /// @throws InvalidArgument if the sampled sigma is negative or zero.
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    mean_->Reset();
    sigma_->Reset();
  }

  /// @warning This is only an approximation.
  inline double Max() { return mean_->Max() + 6 * sigma_->Max(); }
  /// @warning This is only an approximation.
  inline double Min() { return mean_->Min() - 6 * sigma_->Max(); }

 private:
  ExpressionPtr mean_;  ///< Mean value of normal distribution.
  ExpressionPtr sigma_;  ///< Standard deviation of normal distribution.
};

/// @class LogNormalDeviate
/// Log-normal distribution.
class LogNormalDeviate : public Expression {
 public:
  /// Setup for log-normal distribution with validity check for arguments.
  /// @param[in] mean This is the mean of the log-normal distribution not the
  ///                 mean of underlying normal distribution,
  ///                 which is parameter mu. mu is the location parameter,
  ///                 sigma is the scale factor.
  ///                 E(x) = exp(mu + sigma^2 / 2)
  /// @param[in] ef This is the error factor of the log-normal distribution
  ///               for confidence level of 0.95.
  ///               EF = exp(1.645 * sigma)
  /// @param[in] level The confidence level of 0.95 is assumed.
  LogNormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& ef,
                   const ExpressionPtr& level)
      : mean_(mean),
        ef_(ef),
        level_(level) {
    Expression::args_.push_back(mean);
    Expression::args_.push_back(ef);
    Expression::args_.push_back(level);
  }

  /// @throws InvalidArgument if (mean <= 0) or (ef <= 0) or (level != 0.95)
  void Validate();

  inline double Mean() { return mean_->Mean(); }

  /// Samples provided arguments and feeds the Log-normal generator.
  /// @returns A sampled value.
  /// @throws InvalidArgument if (mean <= 0) or (ef <= 0) or (level != 0.95)
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    mean_->Reset();
    ef_->Reset();
    level_->Reset();
  }

  /// 99 percentile estimate.
  inline double Max() {
    double sigma = std::log(ef_->Mean()) / 1.645;
    double mu = std::log(mean_->Max()) - std::pow(sigma, 2) / 2;
    return std::exp(std::sqrt(2) * std::pow(boost::math::erfc(1 / 50), -1) *
                    sigma + mu);
  }

  inline double Min() { return 0; }

 private:
  ExpressionPtr mean_;  ///< Mean value of the log-normal distribution.
  ExpressionPtr ef_;  ///< Error factor of the log-normal distribution.
  ExpressionPtr level_;  ///< Confidence level of the log-normal distribution.
};

/// @class GammaDeviate
/// Gamma distribution.
class GammaDeviate : public Expression {
 public:
  /// Setup for Gamma distribution with validity check for arguments.
  /// @param[in] k Shape parameter of Gamma distribution.
  /// @param[in] theta Scale parameter of Gamma distribution.
  GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta)
      : k_(k),
        theta_(theta) {
    Expression::args_.push_back(k);
    Expression::args_.push_back(theta);
  }

  /// @throws InvalidArgument if (k <= 0) or (theta <= 0)
  void Validate();

  inline double Mean() { return k_->Mean() * theta_->Mean(); }

  /// Samples provided arguments and feeds the gamma distribution generator.
  /// @returns A sampled value.
  /// @throws InvalidArgument if (k <= 0) or (theta <= 0)
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    k_->Reset();
    theta_->Reset();
  }

  inline double Max() {
    return theta_->Max() *
        std::pow(boost::math::gamma_q(k_->Max(),
                                      boost::math::gamma_q(k_->Max(), 0)
                                      - 0.99),
                 -1);
  }

  inline double Min() { return 0; }

 private:
  ExpressionPtr k_;  ///< The shape parameter of the gamma distribution.
  ExpressionPtr theta_;  ///< The scale factor of the gamma distribution.
};

/// @class BetaDeviate
/// Beta distribution.
class BetaDeviate : public Expression {
 public:
  /// Setup for Beta distribution with validity check for arguments.
  /// @param[in] alpha Alpha shape parameter of Gamma distribution.
  /// @param[in] beta Beta shape parameter of Gamma distribution.
  BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta)
      : alpha_(alpha),
        beta_(beta) {
    Expression::args_.push_back(alpha);
    Expression::args_.push_back(beta);
  }

  /// @throws InvalidArgument if (alpha <= 0) or (beta <= 0)
  void Validate();

  inline double Mean() {
    return alpha_->Mean() / (alpha_->Mean() + beta_->Mean());
  }

  /// Samples provided arguments and feeds the beta distribution generator.
  /// @returns A sampled value.
  /// @throws InvalidArgument if (alpha <= 0) or (beta <= 0)
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    alpha_->Reset();
    beta_->Reset();
  }

  /// 99 percentile estimate.
  inline double Max() {
    return std::pow(boost::math::ibeta(alpha_->Max(), beta_->Max(), 0.99), -1);
  }

  inline double Min() { return 0; }

 private:
  ExpressionPtr alpha_;  ///< The alpha shape parameter.
  ExpressionPtr beta_;  ///< The beta shape parameter.
};

/// @class Histogram
/// Histogram distribution.
class Histogram : public Expression {
 public:
  /// Histogram distribution setup.
  /// @param[in] boundaries The upper bounds of intervals.
  /// @param[in] weights The positive weights of intervals restricted by
  ///                    the upper boundaries. Therefore, the number of
  ///                    weights must be equal to the number of boundaries.
  /// @note This description of histogram sampling is for probabilities mostly.
  ///       Therefore, it is not flexible. Currently, it allows sampling both
  ///       boundaries and weights. This behavior makes checking for valid
  ///       arrangement of the boundaries mandatory for each sampling.
  ///       Moreover, the first starting point is assumed but not defined.
  ///       The starting point is assumed to be 0, which leaves only positive
  ///       values for boundaries. This behavior is restrictive and should
  ///       be handled accordingly.
  /// @throws InvalidArgument if boundaries container size is not equal to
  ///                         weights container size.
  Histogram(const std::vector<ExpressionPtr>& boundaries,
            const std::vector<ExpressionPtr>& weights);

  /// @throws InvalidArgument if the boundaries are not strictly increasing
  ///                         or weights are negative.
  void Validate();

  /// Calculates the mean from the histogram.
  double Mean();

  /// Samples the underlying expressions and provides the sampling from
  /// histogram distribution.
  /// @throws InvalidArgument if the boundaries are not strictly increasing
  ///                         or weights are negative.
  double Sample();

  inline void Reset() {
    Expression::sampled_ = false;
    std::vector<ExpressionPtr>::iterator it;
    for (it = boundaries_.begin(); it != boundaries_.end(); ++it) {
      (*it)->Reset();
    }
    for (it = weights_.begin(); it != weights_.end(); ++it) {
      (*it)->Reset();
    }
  }

  inline double Max() { return boundaries_.back()->Max(); }
  inline double Min() { return boundaries_.front()->Min(); }

 private:
  /// Checks if mean values of expressions are strictly increasing.
  /// @throws InvalidArgument if the mean values are not strictly increasing.
  void CheckBoundaries(const std::vector<ExpressionPtr>& boundaries);

  /// Checks if mean values of boundaries are non-negative.
  /// @throws InvalidArgument if the mean values are negative.
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
  /// Construct a new expression that negates a given argument expression.
  /// @param[in] expression The expression to be negated.
  explicit Neg(const ExpressionPtr& expression) : expression_(expression) {
    Expression::args_.push_back(expression);
  }

  inline double Mean() { return -expression_->Mean(); }
  inline double Sample() {
    if (!Expression::sampled_) {
      Expression::sampled_ = true;
      Expression::sampled_value_ = -expression_->Sample();
    }
    return Expression::sampled_value_;
  }
  inline void Reset() {
    Expression::sampled_ = false;
    expression_->Reset();
  }
  inline bool IsConstant() { return expression_->IsConstant(); }
  inline double Max() { return -expression_->Min(); }
  inline double Min() { return -expression_->Max(); }

 private:
  ExpressionPtr expression_;  ///< Expression that is used for negation.
};

/// @class Add
/// This expression adds all the given expressions' values.
class Add : public Expression {
 public:
  /// Construct a new expression that add given argument expressions.
  /// @param[in] arguments The arguments of the addition equation.
  /// @note It is assumed that arguments contain at least one element.
  explicit Add(const std::vector<ExpressionPtr>& arguments) {
    Expression::args_ = arguments;
  }

  inline double Mean() {
    assert(!args_.empty());
    double mean = 0;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      mean += (*it)->Mean();
    }
    return mean;
  }
  inline double Sample() {
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
  inline void Reset() {
    assert(!args_.empty());
    Expression::sampled_ = false;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      (*it)->Reset();
    }
  }
  inline bool IsConstant() {
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      if (!(*it)->IsConstant()) return false;
    }
    return true;
  }
  inline double Max() {
    assert(!args_.empty());
    double max = 0;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      max += (*it)->Max();
    }
    return max;
  }
  inline double Min() {
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
  /// Construct a new expression that subtracts given argument expressions
  /// from the first argument expression.
  /// @param[in] arguments The arguments for operation.
  /// @note It is assumed that arguments contain at least one element.
  explicit Sub(const std::vector<ExpressionPtr>& arguments) {
    Expression::args_ = arguments;
  }

  inline double Mean() {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double mean = (*it)->Mean();
    for (++it; it != args_.end(); ++it) {
      mean -= (*it)->Mean();
    }
    return mean;
  }
  inline double Sample() {
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
  inline void Reset() {
    assert(!args_.empty());
    Expression::sampled_ = false;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      (*it)->Reset();
    }
  }
  inline bool IsConstant() {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      if (!(*it)->IsConstant()) return false;
    }
    return true;
  }
  inline double Max() {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double max = (*it)->Max();
    for (++it; it != args_.end(); ++it) {
      max -= (*it)->Min();
    }
    return max;
  }
  inline double Min() {
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
  /// Construct a new expression that multiplies given argument expressions.
  /// @param[in] arguments The arguments for operation.
  /// @note It is assumed that arguments contain at least one element.
  explicit Mul(const std::vector<ExpressionPtr>& arguments) {
    Expression::args_ = arguments;
  }

  inline double Mean() {
    assert(!args_.empty());
    double mean = 1;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      mean *= (*it)->Mean();
    }
    return mean;
  }
  inline double Sample() {
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
  inline void Reset() {
    assert(!args_.empty());
    Expression::sampled_ = false;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      (*it)->Reset();
    }
  }
  inline bool IsConstant() {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      if (!(*it)->IsConstant()) return false;
    }
    return true;
  }
  /// Finds maximum product from the given arguments' minimum and maximum
  /// values. Negative values may introduce sign cancellation.
  /// @returns Maximum possible value of the product.
  inline double Max() {
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
  /// Finds minimum product from the given arguments' minimum and maximum
  /// values. Negative values may introduce sign cancellation.
  /// @returns Minimum possible value of the product.
  inline double Min() {
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
class Div : public Expression {
 public:
  /// Construct a new expression that divides the first given argument
  /// by the rest of argument expressions.
  /// @param[in] arguments The arguments for operation.
  /// @note It is assumed that arguments contain at least one element.
  ///       No arguments except the first should be 0. The bevaior may be
  ///       undefined if the value is 0 for division.
  explicit Div(const std::vector<ExpressionPtr>& arguments) {
    Expression::args_ = arguments;
  }

  inline double Mean() {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it = args_.begin();
    double mean = (*it)->Mean();
    for (++it; it != args_.end(); ++it) {
      mean /= (*it)->Mean();
    }
    return mean;
  }
  inline double Sample() {
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
  inline void Reset() {
    assert(!args_.empty());
    Expression::sampled_ = false;
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      (*it)->Reset();
    }
  }
  inline bool IsConstant() {
    assert(!args_.empty());
    std::vector<ExpressionPtr>::iterator it;
    for (it = args_.begin(); it != args_.end(); ++it) {
      if (!(*it)->IsConstant()) return false;
    }
    return true;
  }
  /// Finds maximum results of division of the given arguments'
  /// minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  /// @returns Maximum value for division of arguments.
  inline double Max() {
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
  /// Finds minimum results of division of the given arguments'
  /// minimum and maximum values.
  /// Negative values may introduce sign cancellation.
  /// @returns Minimum value for division of arguments.
  inline double Min() {
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
