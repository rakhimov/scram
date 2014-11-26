/// @file expression.cc
/// Implementation of various expressions for basic event probability
/// description.
#include "expression.h"

#include "error.h"
#include "random.h"

namespace scram {

void Parameter::Validate() {
  std::vector<std::string> path;
  Parameter::CheckCyclicity(&path);
}

void Parameter::CheckCyclicity(std::vector<std::string>* path) {
  std::vector<std::string>::iterator it = std::find(path->begin(),
                                                    path->end(),
                                                    name_);
  if (it != path->end()) {
    std::string msg = "Detected a cyclicity through '" + name_ +
                      "' parameter:\n";
    msg += name_;
    for (++it; it != path->end(); ++it) {
      msg += "->" + *it;
    }
    msg += "->" + name_;
    throw ValidationError(msg);
  }
  path->push_back(name_);
  boost::shared_ptr<scram::Parameter> ptr =
      boost::dynamic_pointer_cast<Parameter>(expression_);
  if (ptr) ptr->CheckCyclicity(path);
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

double ExponentialExpression::Sample() {
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
    throw InvalidArgument("Invalid value for probabilty.");
  } else if (time_->Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  } else if (lambda_->Min() < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (mu_->Min() < 0) {
    throw InvalidArgument("The sampled rate of repair cannot be negative.");
  } else if (gamma_->Min() < 0 || gamma_->Max() > 1) {
    throw InvalidArgument("Invalid sampled gamma value for probabilty.");
  } else if (time_->Min() < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }
}

double GlmExpression::Sample() {
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

double WeibullExpression::Sample() {
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

void UniformDeviate::Validate() {
  if (min_->Mean() >= max_->Mean()) {
    throw InvalidArgument("Min value is more than max for Uniform"
                          " distribution.");
  } else if (min_->Max() >= max_->Min()) {
    throw InvalidArgument("Sampled min value is more than sampled max"
                          " for Uniform distribution.");
  }
}

double UniformDeviate::Sample() {
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

double NormalDeviate::Sample() {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    Expression::sampled_value_ =  Random::NormalGenerator(mean_->Sample(),
                                                          sigma_->Sample());
  }
  return Expression::sampled_value_;
}

void LogNormalDeviate::Validate() {
  if (level_->Mean() != 0.95) {
    throw InvalidArgument("The confidence level is expected to be 0.95.");
  } else if (ef_->Mean() <= 1) {
    throw InvalidArgument("The Error Factor for Log-Normal distribution"
                          " cannot be less than 1.");
  } else if (mean_->Mean() <= 0) {
    throw InvalidArgument("The mean of Log-Normal distribution cannot be"
                          " negative or zero.");
  } else if (ef_->Min() <= 1) {
    throw InvalidArgument("The Sampled Error Factor for Log-Normal"
                          " distribution cannot be less than 1.");
  } else if (mean_->Min() <= 0) {
    throw InvalidArgument("The sampled mean of Log-Normal distribution"
                          " cannot be negative or zero.");
  }
}

double LogNormalDeviate::Sample() {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    double sigma = std::log(ef_->Sample()) / 1.645;
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

double GammaDeviate::Sample() {
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

double BetaDeviate::Sample() {
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
}

void Histogram::Validate() {
  CheckBoundaries(boundaries_);
  CheckWeights(weights_);
}

double Histogram::Mean() {
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

double Histogram::Sample() {
  if (!Expression::sampled_) {
    Expression::sampled_ = true;
    std::vector<double> b;
    b.push_back(0);  // The initial point.
    std::vector<double> w;
    for (int i = 0; i < boundaries_.size(); ++i) {
      b.push_back(boundaries_[i]->Sample());
      w.push_back(weights_[i]->Sample());
    }
    Expression::sampled_value_ = Random::HistogramGenerator(w, b);
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
