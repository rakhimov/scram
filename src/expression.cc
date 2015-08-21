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

Expression::Expression(const std::vector<ExpressionPtr>& args) : args_(args) {}

void Expression::Reset() noexcept {
  if (!Expression::sampled_) return;
  Expression::sampled_ = false;
  for (ExpressionPtr arg : args_) arg->Reset();
}

bool Expression::IsConstant() noexcept {
  for (ExpressionPtr arg : args_) {
    if (!arg->IsConstant()) return false;
  }
  return true;
}

void Expression::GatherNodesAndConnectors() {
  assert(nodes_.empty());
  assert(connectors_.empty());
  std::vector<ExpressionPtr>::iterator it;
  for (it = args_.begin(); it != args_.end(); ++it) {
    Parameter* ptr = dynamic_cast<Parameter*>(it->get());
    if (ptr) {
      nodes_.push_back(ptr);
    } else {
      connectors_.push_back(it->get());
    }
  }
  gather_ = false;
}

Parameter::Parameter(const std::string& name, const std::string& base_path,
                     bool is_public)
      : Role::Role(is_public, base_path),
        name_(name),
        unit_(kUnitless),
        mark_(""),
        unused_(true) {
  assert(name != "");
  id_ = is_public ? name : base_path + "." + name;  // Unique combination.
  boost::to_lower(id_);
}

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
    Expression::sampled_value_ =  1 - std::exp(-(lambda_->Sample() *
                                                 time_->Sample()));
  }
  return Expression::sampled_value_;
}

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

double GlmExpression::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    double gamma = gamma_->Sample();
    double lambda = lambda_->Sample();
    double mu = mu_->Sample();
    double time = time_->Sample();
    double r = lambda + mu;
    Expression::sampled_value_ =  (lambda - (lambda - gamma * r) *
                                   std::exp(-r * time)) / r;
  }
  return Expression::sampled_value_;
}

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
    double alpha = alpha_->Sample();
    double beta = beta_->Sample();
    double t0 = t0_->Sample();
    double time = time_->Sample();
    Expression::sampled_value_ =  1 - std::exp(-std::pow((time - t0) /
                                                         alpha, beta));
  }

  return Expression::sampled_value_;
}

RandomDeviate::~RandomDeviate() {}  // Empty destructor for the abstract class.

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
    double l = level_->Sample();
    double p = l + (1 - l) / 2;
    double z = std::sqrt(2) * boost::math::erfc_inv(2 * p);
    z = std::abs(z);
    double sigma = std::log(ef_->Sample()) / z;
    double mu = std::log(mean_->Sample()) - std::pow(sigma, 2) / 2;
    Expression::sampled_value_ =  Random::LogNormalGenerator(mu, sigma);
  }
  return Expression::sampled_value_;
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

double GammaDeviate::Sample() noexcept {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ = Random::GammaGenerator(k_->Sample(),
                                                        theta_->Sample());
  }
  return Expression::sampled_value_;
}

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

Histogram::Histogram(const std::vector<ExpressionPtr>& boundaries,
                     const std::vector<ExpressionPtr>& weights) {
  if (weights.size() != boundaries.size()) {
    throw InvalidArgument("The number of weights is not equal to the number"
                          " of boundaries.");
  }
  boundaries_ = boundaries;
  weights_ = weights;
  Expression::args_.insert(Expression::args_.end(), boundaries.begin(),
                           boundaries.end());
  Expression::args_.insert(Expression::args_.end(), weights.begin(),
                           weights.end());
}

void Histogram::Validate() {
  CheckBoundaries(boundaries_);
  CheckWeights(weights_);
}

double Histogram::Mean() noexcept {
  double sum_weights = 0;
  double sum_product = 0;
  double lower_bound = 0;
  for (int i = 0; i < boundaries_.size(); ++i) {
    sum_product += (boundaries_[i]->Mean() - lower_bound) *
                    weights_[i]->Mean();
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

}  // namespace scram
