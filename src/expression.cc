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

/// @file expression.cc
/// Implementation of various expressions
/// for basic event probability description.

#include "expression.h"

#include <boost/algorithm/string.hpp>

#include "error.h"
#include "random.h"

namespace scram {

Expression::Expression(std::vector<ExpressionPtr> args)
    : args_(std::move(args)),
      sampled_value_(0),
      sampled_(false),
      gather_(true) {}

void Expression::Reset() noexcept {
  if (!Expression::sampled_) return;
  Expression::sampled_ = false;
  for (const ExpressionPtr& arg : args_) arg->Reset();
}

bool Expression::IsConstant() noexcept {
  for (const ExpressionPtr& arg : args_) {
    if (!arg->IsConstant()) return false;
  }
  return true;
}

void Expression::GatherNodesAndConnectors() {
  assert(nodes_.empty());
  assert(connectors_.empty());
  for (const ExpressionPtr& arg : args_) {
    Parameter* ptr = dynamic_cast<Parameter*>(arg.get());
    if (ptr) {
      nodes_.push_back(ptr);
    } else {
      connectors_.push_back(arg.get());
    }
  }
  gather_ = false;
}

Parameter::Parameter(const std::string& name, const std::string& base_path,
                     bool is_public)
      : Expression::Expression({}),
        Role::Role(is_public, base_path),
        name_(name),
        unit_(kUnitless),
        unused_(true),
        mark_("") {
  assert(name != "");
  id_ = is_public ? name : base_path + "." + name;  // Unique combination.
  boost::to_lower(id_);
}

MissionTime::MissionTime()
      : Expression::Expression({}),
        mission_time_(-1),
        unit_(kHours) {}

ConstantExpression::ConstantExpression(double val)
      : Expression::Expression({}),
        value_(val) {}

ConstantExpression::ConstantExpression(int val)
      : Expression::Expression({}),
        value_(val) {}

ConstantExpression::ConstantExpression(bool val)
      : Expression::Expression({}),
        value_(val) {}

ExponentialExpression::ExponentialExpression(const ExpressionPtr& lambda,
                                             const ExpressionPtr& t)
    : Expression::Expression({lambda, t}),
      lambda_(lambda),
      time_(t) {}

void ExponentialExpression::Validate() {
  if (lambda_->Mean() < 0) {
    throw InvalidArgument("The rate of failure cannot be negative.");
  } else if (time_->Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (lambda_->Min() < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (time_->Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }
}

double ExponentialExpression::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ =
        1 - std::exp(-(lambda_->Sample() * time_->Sample()));
  }
  return Expression::sampled_value_;
}

GlmExpression::GlmExpression(const ExpressionPtr& gamma,
                             const ExpressionPtr& lambda,
                             const ExpressionPtr& mu,
                             const ExpressionPtr& t)
    : Expression::Expression({gamma, lambda, mu, t}),
      gamma_(gamma),
      lambda_(lambda),
      mu_(mu),
      time_(t) {}

void GlmExpression::Validate() {
  if (lambda_->Mean() < 0) {
    throw InvalidArgument("The rate of failure cannot be negative.");
  } else if (mu_->Mean() < 0) {
    throw InvalidArgument("The rate of repair cannot be negative.");
  } else if (gamma_->Mean() < 0 || gamma_->Mean() > 1) {
    throw InvalidArgument("Invalid value for probability.");
  } else if (time_->Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (lambda_->Min() < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (mu_->Min() < 0) {
    throw InvalidArgument("The sampled rate of repair cannot be negative.");
  } else if (gamma_->Min() < 0 || gamma_->Max() > 1) {
    throw InvalidArgument("Invalid sampled gamma value for probability.");
  } else if (time_->Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }
}

double GlmExpression::Mean() noexcept {
  return GlmExpression::Compute(gamma_->Mean(), lambda_->Mean(), mu_->Mean(),
                                time_->Mean());
}

double GlmExpression::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ =
        GlmExpression::Compute(gamma_->Sample(), lambda_->Sample(),
                               mu_->Sample(), time_->Sample());
  }
  return Expression::sampled_value_;
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
    : Expression::Expression({alpha, beta, t0, time}),
      alpha_(alpha),
      beta_(beta),
      t0_(t0),
      time_(time) {}

void WeibullExpression::Validate() {
  if (alpha_->Mean() <= 0) {
    throw InvalidArgument("The scale parameter for Weibull distribution must"
                          " be positive.");
  } else if (beta_->Mean() <= 0) {
    throw InvalidArgument("The shape parameter for Weibull distribution must"
                          " be positive.");
  } else if (t0_->Mean() < 0) {
    throw InvalidArgument("Invalid value for time shift.");
  } else if (time_->Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (time_->Mean() < t0_->Mean()) {
    throw InvalidArgument("The mission time must be longer than time shift.");
  } else if (alpha_->Min() <= 0) {
    throw InvalidArgument("The scale parameter for Weibull distribution must"
                          " be positive for sampled values.");
  } else if (beta_->Min() <= 0) {
    throw InvalidArgument("The shape parameter for Weibull distribution must"
                          " be positive for sampled values.");
  } else if (t0_->Min() < 0) {
    throw InvalidArgument("Invalid value for time shift in sampled values.");
  } else if (time_->Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  } else if (time_->Min() < t0_->Max()) {
    throw InvalidArgument("The sampled mission time must be"
                          " longer than time shift.");
  }
}

double WeibullExpression::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ =
        WeibullExpression::Compute(alpha_->Sample(), beta_->Sample(),
                                   t0_->Sample(), time_->Sample());
  }

  return Expression::sampled_value_;
}

double WeibullExpression::Compute(double alpha, double beta,
                                  double t0, double time) noexcept {
  return 1 - std::exp(-std::pow((time - t0) / alpha, beta));
}

RandomDeviate::~RandomDeviate() {}  // Empty destructor for the abstract class.

UniformDeviate::UniformDeviate(const ExpressionPtr& min,
                               const ExpressionPtr& max)
      : RandomDeviate::RandomDeviate({min, max}),
        min_(min),
        max_(max) {}

void UniformDeviate::Validate() {
  if (min_->Mean() >= max_->Mean()) {
    throw InvalidArgument("Min value is more than max for Uniform"
                          " distribution.");
  } else if (min_->Max() >= max_->Min()) {
    throw InvalidArgument("Sampled min value is more than sampled max"
                          " for Uniform distribution.");
  }
}

double UniformDeviate::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ = Random::UniformRealGenerator(min_->Sample(),
                                                              max_->Sample());
  }
  return Expression::sampled_value_;
}

NormalDeviate::NormalDeviate(const ExpressionPtr& mean,
                             const ExpressionPtr& sigma)
      : RandomDeviate::RandomDeviate({mean, sigma}),
        mean_(mean),
        sigma_(sigma) {}

void NormalDeviate::Validate() {
  if (sigma_->Mean() <= 0) {
    throw InvalidArgument("Standard deviation cannot be negative or zero.");
  } else if (sigma_->Min() <= 0) {
    throw InvalidArgument("Sampled standard deviation is negative or zero.");
  }
}

double NormalDeviate::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ =  Random::NormalGenerator(mean_->Sample(),
                                                          sigma_->Sample());
  }
  return Expression::sampled_value_;
}

LogNormalDeviate::LogNormalDeviate(const ExpressionPtr& mean,
                                   const ExpressionPtr& ef,
                                   const ExpressionPtr& level)
      : RandomDeviate::RandomDeviate({mean, ef, level}),
        mean_(mean),
        ef_(ef),
        level_(level) {}

void LogNormalDeviate::Validate() {
  if (level_->Mean() <= 0 || level_->Mean() >= 1) {
    throw InvalidArgument("The confidence level is not within (0, 1).");
  } else if (ef_->Mean() <= 1) {
    throw InvalidArgument("The Error Factor for Log-Normal distribution"
                          " cannot be less than 1.");
  } else if (mean_->Mean() <= 0) {
    throw InvalidArgument("The mean of Log-Normal distribution cannot be"
                          " negative or zero.");
  } else if (level_->Min() <= 0 || level_->Max() >= 1) {
    throw InvalidArgument("The confidence level doesn't sample within (0, 1).");

  } else if (ef_->Min() <= 1) {
    throw InvalidArgument("The Sampled Error Factor for Log-Normal"
                          " distribution cannot be less than 1.");
  } else if (mean_->Min() <= 0) {
    throw InvalidArgument("The sampled mean of Log-Normal distribution"
                          " cannot be negative or zero.");
  }
}

double LogNormalDeviate::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    double sigma =
        LogNormalDeviate::ComputeScale(level_->Sample(), ef_->Sample());
    double mu =
        LogNormalDeviate::ComputeLocation(mean_->Sample(), sigma);
    Expression::sampled_value_ =  Random::LogNormalGenerator(mu, sigma);
  }
  return Expression::sampled_value_;
}

double LogNormalDeviate::Max() noexcept {
  double sigma = LogNormalDeviate::ComputeScale(level_->Mean(), ef_->Mean());
  double mu = LogNormalDeviate::ComputeLocation(mean_->Max(), sigma);
  return std::exp(
      std::sqrt(2) * std::pow(boost::math::erfc(1 / 50), -1) * sigma + mu);
}

double LogNormalDeviate::ComputeScale(double level, double ef) noexcept {
  double p = level + (1 - level) / 2;
  double z = std::sqrt(2) * boost::math::erfc_inv(2 * p);
  return std::log(ef) / std::abs(z);
}

double LogNormalDeviate::ComputeLocation(double mean, double sigma) noexcept {
  return std::log(mean) - std::pow(sigma, 2) / 2;
}

void GammaDeviate::Validate() {
  if (k_->Mean() <= 0) {
    throw InvalidArgument("The k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta_->Mean() <= 0) {
    throw InvalidArgument("The theta scale parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (k_->Min() <= 0) {
    throw InvalidArgument("Sampled k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta_->Min() <= 0) {
    throw InvalidArgument("Sampled theta scale parameter for Gamma "
                          "distribution cannot be negative or zero.");
  }
}

GammaDeviate::GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta)
      : RandomDeviate::RandomDeviate({k, theta}),
        k_(k),
        theta_(theta) {}

double GammaDeviate::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ = Random::GammaGenerator(k_->Sample(),
                                                        theta_->Sample());
  }
  return Expression::sampled_value_;
}

BetaDeviate::BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta)
      : RandomDeviate::RandomDeviate({alpha, beta}),
        alpha_(alpha),
        beta_(beta) {}

void BetaDeviate::Validate() {
  if (alpha_->Mean() <= 0) {
    throw InvalidArgument("The alpha shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (beta_->Mean() <= 0) {
    throw InvalidArgument("The beta shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (alpha_->Min() <= 0) {
    throw InvalidArgument("Sampled alpha shape parameter for"
                          " Beta distribution cannot be negative or zero.");
  } else if (beta_->Min() <= 0) {
    throw InvalidArgument("Sampled beta shape parameter for Beta"
                          " distribution cannot be negative or zero.");
  }
}

double BetaDeviate::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ = Random::BetaGenerator(alpha_->Sample(),
                                                       beta_->Sample());
  }
  return Expression::sampled_value_;
}

Histogram::Histogram(std::vector<ExpressionPtr> boundaries,
                     std::vector<ExpressionPtr> weights)
    : RandomDeviate::RandomDeviate(boundaries),
      boundaries_(std::move(boundaries)),
      weights_(std::move(weights)) {
  if (weights_.size() != boundaries_.size()) {
    throw InvalidArgument("The number of weights is not equal to the number"
                          " of boundaries.");
  }
  Expression::args_.insert(Expression::args_.end(), weights_.begin(),
                           weights_.end());
}

void Histogram::Validate() {
  Histogram::CheckBoundaries(boundaries_);
  Histogram::CheckWeights(weights_);
}

double Histogram::Mean() noexcept {
  double sum_weights = 0;
  double sum_product = 0;
  double lower_bound = 0;
  for (int i = 0; i < boundaries_.size(); ++i) {
    sum_product += (boundaries_[i]->Mean() - lower_bound) * weights_[i]->Mean();
    sum_weights += weights_[i]->Mean();
    lower_bound = boundaries_[i]->Mean();
  }
  return sum_product / (lower_bound * sum_weights);
}

double Histogram::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    std::vector<double> sampled_boundaries;
    sampled_boundaries.push_back(0);  // The initial point.
    std::vector<double> sampled_weights;
    for (int i = 0; i < boundaries_.size(); ++i) {
      sampled_boundaries.push_back(boundaries_[i]->Sample());
      sampled_weights.push_back(weights_[i]->Sample());
    }
    Expression::sampled_value_ = Random::HistogramGenerator(sampled_boundaries,
                                                            sampled_weights);
  }
  return Expression::sampled_value_;
}

void Histogram::CheckBoundaries(const std::vector<ExpressionPtr>& boundaries) {
  if (boundaries[0]->Mean() <= 0) {
    throw InvalidArgument("Histogram lower boundary must be positive.");
  } else if (boundaries[0]->Min() <= 0) {
    throw InvalidArgument("Histogram sampled lower boundary must be positive.");
  }
  for (int i = 1; i < boundaries.size(); ++i) {
    if (boundaries[i-1]->Mean() >= boundaries[i]->Mean()) {
      throw InvalidArgument("Histogram upper boundaries are not strictly"
                            " increasing.");
    } else if (boundaries[i-1]->Max() >= boundaries[i]->Min()) {
      throw InvalidArgument("Histogram sampled upper boundaries must"
                            " be strictly increasing.");
    }
  }
}

void Histogram::CheckWeights(const std::vector<ExpressionPtr>& weights) {
  for (int i = 0; i < weights.size(); ++i) {
    if (weights[i]->Mean() < 0) {
      throw InvalidArgument("Histogram weights are negative.");
    } else if (weights[i]->Min() < 0) {
      throw InvalidArgument("Histogram sampled weights are negative.");
    }
  }
}

Neg::Neg(const ExpressionPtr& expression)
      : Expression::Expression({expression}),
        expression_(expression) {}

double Add::Mean() noexcept {
  assert(!args_.empty());
  double mean = 0;
  for (const ExpressionPtr& arg : args_) mean += arg->Mean();
  return mean;
}

double Add::Sample() noexcept {
  assert(!args_.empty());
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ = 0;
    for (const ExpressionPtr& arg : args_)
      Expression::sampled_value_ += arg->Sample();
  }
  return Expression::sampled_value_;
}

double Add::Max() noexcept {
  assert(!args_.empty());
  double max = 0;
  for (const ExpressionPtr& arg : args_) max += arg->Max();
  return max;
}

double Add::Min() noexcept {
  assert(!args_.empty());
  double min = 0;
  for (const ExpressionPtr& arg : args_) min += arg->Min();
  return min;
}

double Sub::Mean() noexcept {
  assert(!args_.empty());
  std::vector<ExpressionPtr>::iterator it = args_.begin();
  double mean = (*it)->Mean();
  for (++it; it != args_.end(); ++it) {
    mean -= (*it)->Mean();
  }
  return mean;
}

double Sub::Sample() noexcept {
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

double Sub::Max() noexcept {
  assert(!args_.empty());
  std::vector<ExpressionPtr>::iterator it = args_.begin();
  double max = (*it)->Max();
  for (++it; it != args_.end(); ++it) {
    max -= (*it)->Min();
  }
  return max;
}

double Sub::Min() noexcept {
  assert(!args_.empty());
  std::vector<ExpressionPtr>::iterator it = args_.begin();
  double min = (*it)->Min();
  for (++it; it != args_.end(); ++it) {
    min -= (*it)->Max();
  }
  return min;
}

double Mul::Mean() noexcept {
  assert(!args_.empty());
  double mean = 1;
  for (const ExpressionPtr& arg : args_) mean *= arg->Mean();
  return mean;
}

double Mul::Sample() noexcept {
  assert(!args_.empty());
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ = 1;
    for (const ExpressionPtr& arg : args_)
      Expression::sampled_value_ *= arg->Sample();
  }
  return Expression::sampled_value_;
}

double Mul::GetExtremum(bool maximum) noexcept {
  double max_val = 1;  // Maximum possible product.
  double min_val = 1;  // Minimum possible product.
  for (const ExpressionPtr& arg : args_) {
    double mult_max = arg->Max();
    double mult_min = arg->Min();
    double max_max = max_val * mult_max;
    double max_min = max_val * mult_min;
    double min_max = min_val * mult_max;
    double min_min = min_val * mult_min;
    max_val = std::max({max_max, max_min, min_max, min_min});
    min_val = std::min({max_max, max_min, min_max, min_min});
  }
  if (maximum) return max_val;
  return min_val;
}

void Div::Validate() {
  assert(!args_.empty());
  std::vector<ExpressionPtr>::iterator it = args_.begin();
  for (++it; it != args_.end(); ++it) {
    const auto& expr = *it;
    if (!expr->Mean() || !expr->Max() || !expr->Min())
      throw InvalidArgument("Division by 0.");
  }
}

double Div::Mean() noexcept {
  assert(!args_.empty());
  std::vector<ExpressionPtr>::iterator it = args_.begin();
  double mean = (*it)->Mean();
  for (++it; it != args_.end(); ++it) {
    mean /= (*it)->Mean();
  }
  return mean;
}

double Div::Sample() noexcept {
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

double Div::GetExtremum(bool maximum) noexcept {
  assert(!args_.empty());
  std::vector<ExpressionPtr>::iterator it = args_.begin();
  double max_value = (*it)->Max();  // Maximum possible result.
  double min_value = (*it)->Min();  // Minimum possible result.
  for (++it; it != args_.end(); ++it) {
    double div_max = (*it)->Max();
    double div_min = (*it)->Min();
    double max_max = max_value / div_max;
    double max_min = max_value / div_min;
    double min_max = min_value / div_max;
    double min_min = min_value / div_min;
    max_value = std::max({max_max, max_min, min_max, min_min});
    min_value = std::min({max_max, max_min, min_max, min_min});
  }
  if (maximum) return max_value;
  return min_value;
}

}  // namespace scram
