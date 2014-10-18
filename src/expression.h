/// @file expression.h
/// Expressions that describe basic events.
#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <cmath>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "element.h"
#include "random.h"

namespace scram {

/// @class Expression
/// The base class for all sorts of expressions to describe events.
class Expression {
 public:
  virtual ~Expression() {}

  /// @returns The mean value of this expression.
  virtual double Mean() = 0;

  /// @returns A sampled value of this expression.
  virtual double Sample() = 0;
};

/// @class ConstantExpression
/// Indicates a constant value.
class ConstantExpression : public Expression {
 public:
  /// Constructor for integer and float values.
  /// @param[in] val Any numerical value.
  explicit ConstantExpression(double val) : value_(val) {}

  /// Constructor for boolean values.
  /// @param[in] val true for 1 and false for 0 value of this constant.
  explicit ConstantExpression(bool val) : value_(val ? 1 : 0) {}

  inline double Mean() { return value_; }
  inline double Sample() { return value_; }

 private:
  /// The constant's value.
  double value_;
};

typedef boost::shared_ptr<scram::Expression> ExpressionPtr;

/// @class ExponentialExpression
/// Negative exponential distribution with hourly failure rate and time.
class ExponentialExpression : public Expression {
 public:
  /// Constructor for exponential expression with two arguments.
  /// @param[in] lambda Hourly rate of failure.
  /// @param[in] t Mission time in hours.
  /// @throws InvalidArgument if arguments are negative.
  ExponentialExpression(const ExpressionPtr& lambda, const ExpressionPtr& t);

  inline double Mean() {
    return 1 - std::exp(-(lambda_->Mean() * time_->Mean()));
  }

  /// Samples the underlying distributions.
  /// @returns A sampled value.
  /// @throws InvalidArgument if sampled failure rate or time is negative.
  double Sample();

 private:
  /// Failure rate in hours.
  ExpressionPtr lambda_;

  /// Mission time in hours.
  ExpressionPtr time_;
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
  /// @throws InvalidArgument if arguments are invalid.
  GlmExpression(const ExpressionPtr& gamma, const ExpressionPtr& lambda,
                const ExpressionPtr& mu, const ExpressionPtr& t);

  inline double Mean() {
    double r = lambda_->Mean() + mu_->Mean();
    return (lambda_->Mean() - (lambda_->Mean() - gamma_->Mean() * r) *
                               std::exp(-r * time_->Mean())) / r;
  }

  /// Samples the underlying distributions.
  /// @returns A sampled value.
  /// @throws InvalidArgument if sampled values are invalid.
  double Sample();

 private:
  /// Failure rate in hours.
  ExpressionPtr gamma_;

  /// Failure rate in hours.
  ExpressionPtr lambda_;

  /// Mission time in hours.
  ExpressionPtr time_;

  /// Mission time in hours.
  ExpressionPtr mu_;
};

/// @class WeibullExpression
/// Weibull distribution with scale, shap, time shift, and time.
class WeibullExpression : public Expression {
 public:
  /// Constructor for Wibull distribution.
  /// @param[in] alpha Scale parameter.
  /// @param[in] beta Shape parameter.
  /// @param[in] t0 Time shift.
  /// @param[in] time Mission time.
  /// @throws InvalidArgument if arguments are invalid.
  WeibullExpression(const ExpressionPtr& alpha, const ExpressionPtr& beta,
                    const ExpressionPtr& t0, const ExpressionPtr& time);

  inline double Mean() {
    return 1 - std::exp(-std::pow((time_->Mean() - t0_->Mean()) /
                                  alpha_->Mean(), beta_->Mean()));
  }

  /// Samples the underlying distributions.
  /// @returns A sampled value.
  /// @throws InvalidArgument if sampled values are invalid.
  double Sample();

 private:
  /// Scale parameter.
  ExpressionPtr alpha_;

  /// Shape parameter.
  ExpressionPtr beta_;

  /// Time shift in hours.
  ExpressionPtr t0_;

  /// Mission time in hours.
  ExpressionPtr time_;
};

/// @enum Units
/// Provides units for parameters.
enum Units {
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
class Parameter : public Expression, public Element {
 public:
  /// Sets the expression of this basic event.
  /// @param[in] name The name of this variable (Case sensitive).
  /// @param[in] expression The expression to describe this event.
  explicit Parameter(std::string name) : name_(name) {}

  /// Sets the expression of this parameter.
  /// @param[in] expression The expression to describe this parameter.
  inline void expression(const ExpressionPtr& expression) {
    expression_ = expression;
  }

  /// @returns The name of this variable.
  inline const std::string& name() { return name_; }

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  /// @returns The unit of this parameter.
  inline const Units& unit() { return unit_; }

  /// Checks for circular references in parameters.
  /// @throws ValidationError if any cyclic reference is found.
  void CheckCyclicity();

  inline double Mean() { return expression_->Mean(); }
  inline double Sample() { return expression_->Sample(); }

 private:
  /// Helper funciton to check for cyclic references in parameters.
  /// @param[out] path The current path of names in cyclicity search.
  /// @throws ValidationError if any cyclic reference is found.
  void CheckCyclicity(std::vector<std::string>* path);

  /// Name of this parameter or variable.
  std::string name_;

  /// Units of this parameter.
  Units unit_;

  /// Expression for this parameter.
  ExpressionPtr expression_;
};

/// @class MissionTime
/// This is for the system mission time.
class MissionTime : public Expression {
 public:
  MissionTime() : mission_time_(-1) {}

  /// Sets the mission time only once.
  /// @param[in] time The mission time.
  void mission_time(double time) {
    assert(time >= 0);
    mission_time_ = time;
  }

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  inline double Mean() { return mission_time_; }
  inline double Sample() { return mission_time_; }

 private:
  /// The constant's value.
  double mission_time_;

  /// Units of this parameter.
  Units unit_;
};

/// @class UniformDeviate
/// Uniform distribution.
class UniformDeviate : public Expression {
 public:
  /// Setup for uniform distribution.
  /// @param[in] min Minimum value of the distribution.
  /// @param[in] max Maximum value of the distribution.
  /// @throws InvalidArgument if min value is more or equal to max value.
  UniformDeviate(const ExpressionPtr& min, const ExpressionPtr& max);

  inline double Mean() { return (min_->Mean() + max_->Mean()) / 2; }

  /// Samples the underlying distributions and uniform distribution.
  /// @returns A sampled value.
  /// @throws InvalidArgument if min value is more or equal to max value.
  double Sample();

 private:
  /// Minimum value of the distribution.
  ExpressionPtr min_;

  /// Maximum value of the distribution.
  ExpressionPtr max_;
};

/// @class NormalDeviate
/// Normal distribution.
class NormalDeviate : public Expression {
 public:
  /// Setup for normal distribution with validity check for arguments.
  /// @param[in] mean The mean of the distribution.
  /// @param[in] sigma The standard deviation of the distribution.
  /// @throws InvalidArgument if sigma is negative or zero.
  NormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& sigma);

  inline double Mean() { return mean_->Mean(); }

  /// Samples the normal distribution.
  /// @returns A sampled value.
  /// @throws InvalidArgument if the sampled sigma is negative or zero.
  double Sample();

 private:
  /// Mean value of normal distribution.
  ExpressionPtr mean_;

  /// Standard deviation of normal distribution.
  ExpressionPtr sigma_;
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
  /// @throws InvalidArgument if (mean <= 0) or (ef <= 0) or (level != 0.95)
  LogNormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& ef,
                   const ExpressionPtr& level);

  inline double Mean() { return mean_->Mean(); }

  /// Samples provided arguments and feeds the Log-normal generator.
  /// @returns A sampled value.
  /// @throws InvalidArgument if (mean <= 0) or (ef <= 0) or (level != 0.95)
  double Sample();

 private:
  /// Mean value of the log-normal distribution.
  ExpressionPtr mean_;

  /// Error factor of the log-normal distribution.
  ExpressionPtr ef_;

  /// Confidence level of the log-normal distribution.
  ExpressionPtr level_;
};

/// @class GammaDeviate
/// Gamma distribution.
class GammaDeviate : public Expression {
 public:
  /// Setup for Gamma distribution with validity check for arguments.
  /// @param[in] k Shape parameter of Gamma distribution.
  /// @param[in] theta Scale parameter of Gamma distribution.
  /// @throws InvalidArgument if (k <= 0) or (theta <= 0)
  GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta);

  inline double Mean() { return k_->Mean() * theta_->Mean(); }

  /// Samples provided arguments and feeds the gamma distribution generator.
  /// @returns A sampled value.
  /// @throws InvalidArgument if (k <= 0) or (theta <= 0)
  double Sample();

 private:
  /// The shape parameter of the gamma distribution.
  ExpressionPtr k_;

  /// The scale factor of the gamma distribution.
  ExpressionPtr theta_;
};

/// @class BetaDeviate
/// Beta distribution.
class BetaDeviate : public Expression {
 public:
  /// Setup for Beta distribution with validity check for arguments.
  /// @param[in] alpha Alpha shape parameter of Gamma distribution.
  /// @param[in] beta Beta shape parameter of Gamma distribution.
  /// @throws InvalidArgument if (alpha <= 0) or (beta <= 0)
  BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta);

  inline double Mean() {
    return alpha_->Mean() / (alpha_->Mean() + beta_->Mean());
  }

  /// Samples provided arguments and feeds the beta distribution generator.
  /// @returns A sampled value.
  /// @throws InvalidArgument if (alpha <= 0) or (beta <= 0)
  double Sample();

 private:
  /// The alpha shape parameter of the beta distribution.
  ExpressionPtr alpha_;

  /// The beta shape parameter of the beta distribution.
  ExpressionPtr beta_;
};

/// @class Histrogram
/// Histrogram distribution.
class Histogram : public Expression {
 public:
  /// Histogram distribution setup.
  /// @param[in] boundaries The upper bounds of intervals.
  /// @param[in] weights The positive weights of intervals restricted by
  ///                    the upper boundaries. Therefore, the number of
  ///                    weights must be equal to the number of boundaries.
  /// @todo This description is too flexible and ambiguous;
  ///       It allows sampling both
  ///       boundaries and weights. This behavior makes checking for valid
  ///       arrangement of the boundaries mandatory for each sampling.
  ///       Moreover, the first starting point is assumed but not defined.
  ///       The starting point is assumed to be 0, which leaves only positive
  ///       values for boundaries. This behavior is restrictive and should
  ///       be handled differently.
  /// @throws InvalidArgument if the boundaries are not strictly increasing
  ///                         or weights are negative.
  Histogram(const std::vector<ExpressionPtr>& boundaries,
            const std::vector<ExpressionPtr>& weights);

  /// Calculates the mean from the histogram.
  /// @throws InvalidArgument if the boundaries are not strictly increasing
  ///                         or weights are negative.
  double Mean();

  /// Samples the underlying expressions and provides the sampling from
  /// histogram distribution.
  /// @throws InvalidArgument if the boundaries are not strictly increasing
  ///                         or weights are negative.
  double Sample();

 private:
  /// Checks if mean values of expressions are stricly increasing.
  /// @throws InvalidArgument if the mean values are not strictly increasing.
  void CheckBoundaries(const std::vector<ExpressionPtr>& boundaries);

  /// Checks if values are stricly increasing for sampled values.
  /// @throws InvalidArgument if the values are not strictly increasing.
  void CheckBoundaries(const std::vector<double>& boundaries);

  /// Checks if mean values of boundaries are non-negative.
  /// @throws InvalidArgument if the mean values are negative.
  void CheckWeights(const std::vector<ExpressionPtr>& weights);

  /// Checks if values are non-negative for sampled weights.
  /// @throws InvalidArgument if the values are negative.
  void CheckWeights(const std::vector<double>& weights);

  /// Upper boundaries of the histogram.
  std::vector<ExpressionPtr> boundaries_;

  /// Weights of intervals described by boundaries.
  std::vector<ExpressionPtr> weights_;
};


}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
