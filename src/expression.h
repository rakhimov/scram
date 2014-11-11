/// @file expression.h
/// Expressions that describe basic events.
#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <cmath>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/beta.hpp>

#include "element.h"

namespace scram {

/// @class Expression
/// The base class for all sorts of expressions to describe events.
class Expression {
 public:
  typedef boost::shared_ptr<Expression> ExpressionPtr;

  Expression() : sampled_(false), sampled_value_(0) {}

  virtual ~Expression() {}

  /// Validates the expression.
  /// This late validation is due to parameters that are defined late.
  /// @throws InvalidArgument if arguments are invalid for setup.
  virtual void Validate() = 0;

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

 protected:
  /// Indication if the expression is already sampled.
  bool sampled_;

  /// The sampled value.
  double sampled_value_;
};

/// @enum Units
/// Provides units for parameters.
enum Units {
  kUnitless,
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
  explicit Parameter(std::string name) : name_(name), unit_(kUnitless) {}

  /// Sets the expression of this parameter.
  /// @param[in] expression The expression to describe this parameter.
  inline void expression(const ExpressionPtr& expression) {
    expression_ = expression;
  }

  /// Cyclicity detection invoked.
  /// @throws ValidationError if any cyclic reference is found.
  void Validate();

  /// @returns The name of this variable.
  inline const std::string& name() { return name_; }

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  /// @returns The unit of this parameter.
  inline const Units& unit() { return unit_; }

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
  MissionTime() : mission_time_(-1), unit_(kHours) {}

  /// Sets the mission time only once.
  /// @param[in] time The mission time.
  void mission_time(double time) {
    assert(time >= 0);
    mission_time_ = time;
  }

  void Validate() {}

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  inline double Mean() { return mission_time_; }
  inline double Sample() { return mission_time_; }
  inline bool IsConstant() { return true; }

 private:
  /// The constant's value.
  double mission_time_;

  /// Units of this parameter.
  Units unit_;
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

  void Validate() {}
  inline double Mean() { return value_; }
  inline double Sample() { return value_; }
  inline bool IsConstant() { return true; }

 private:
  /// The constant's value.
  double value_;
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
        time_(t) {}

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
  GlmExpression(const ExpressionPtr& gamma, const ExpressionPtr& lambda,
                const ExpressionPtr& mu, const ExpressionPtr& t)
      : gamma_(gamma),
        lambda_(lambda),
        mu_(mu),
        time_(t) {}

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
/// Weibull distribution with scale, shape, time shift, and time.
class WeibullExpression : public Expression {
 public:
  /// Constructor for Wibull distribution.
  /// @param[in] alpha Scale parameter.
  /// @param[in] beta Shape parameter.
  /// @param[in] t0 Time shift.
  /// @param[in] time Mission time.
  WeibullExpression(const ExpressionPtr& alpha, const ExpressionPtr& beta,
                    const ExpressionPtr& t0, const ExpressionPtr& time)
      : alpha_(alpha),
        beta_(beta),
        t0_(t0),
        time_(time) {}

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
  /// Scale parameter.
  ExpressionPtr alpha_;

  /// Shape parameter.
  ExpressionPtr beta_;

  /// Time shift in hours.
  ExpressionPtr t0_;

  /// Mission time in hours.
  ExpressionPtr time_;
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
        max_(max) {}

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
  NormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& sigma)
      : mean_(mean),
        sigma_(sigma) {}

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
  LogNormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& ef,
                   const ExpressionPtr& level)
      : mean_(mean),
        ef_(ef),
        level_(level) {}

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
  GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta)
      : k_(k),
        theta_(theta) {}

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
  BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta)
      : alpha_(alpha),
        beta_(beta) {}

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
  /// The alpha shape parameter of the beta distribution.
  ExpressionPtr alpha_;

  /// The beta shape parameter of the beta distribution.
  ExpressionPtr beta_;
};

/// @class Histogram
/// Histrogram distribution.
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
  /// Checks if mean values of expressions are stricly increasing.
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

}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
