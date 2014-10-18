/// @file expression.cc
/// Implementation of various expressions for basic event probability
/// description.
#include "expression.h"

#include <algorithm>

#include "error.h"

typedef boost::shared_ptr<scram::Parameter> ParameterPtr;

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
  ParameterPtr ptr = boost::dynamic_pointer_cast<Parameter>(expression_);
  if (ptr) ptr->CheckCyclicity(path);
}

void ExponentialExpression::Validate() {
  if (lambda_->Mean() < 0) {
    throw InvalidArgument("The rate of failure cannot be negative.");
  } else if (time_->Mean() < 0) {
    throw InvalidArgument("The mission time cannot be negative.");
  }
}

double ExponentialExpression::Sample() {
  double lambda = lambda_->Sample();
  double time = time_->Sample();

  if (lambda < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (time < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }
  return 1 - std::exp(-(lambda * time));
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
  }
}

double GlmExpression::Sample() {
  double gamma = gamma_->Sample();
  double lambda = lambda_->Sample();
  double mu = mu_->Sample();
  double time = time_->Sample();

  if (lambda < 0) {
    throw InvalidArgument("The sampled rate of failure cannot be negative.");
  } else if (mu < 0) {
    throw InvalidArgument("The sampled rate of repair cannot be negative.");
  } else if (gamma < 0 || gamma > 1) {
    throw InvalidArgument("Invalid sampled value for probabilty.");
  } else if (time < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  }

  double r = lambda + mu;
  return (lambda - (lambda - gamma * r) * std::exp(-r * time)) / r;
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
  }
}

double WeibullExpression::Sample() {
  double alpha = alpha_->Sample();
  double beta = beta_->Sample();
  double t0 = t0_->Sample();
  double time = time_->Sample();

  if (alpha <= 0) {
    throw InvalidArgument("The scale parameter for Weibull distribution must"
                          " be positive for sampled values.");
  } else if (beta <= 0) {
    throw InvalidArgument("The shape parameter for Weibull distribution must"
                          " be positive for sampled values.");
  } else if (t0 < 0) {
    throw InvalidArgument("Invalid value for time shift in sampled values.");
  } else if (time < 0) {
    throw InvalidArgument("The sampled mission time cannot be negative.");
  } else if (time < t0) {
    throw InvalidArgument("The sampled mission time must be"
                          " longer than time shift.");
  }

  return 1 - std::exp(-std::pow((time - t0) / alpha, beta));
}

void UniformDeviate::Validate() {
  if (min_->Mean() >= max_->Mean()) {
    throw InvalidArgument("Min value is more than max for Uniform"
                          " distribution.");
  }
}

double UniformDeviate::Sample() {
  double min = min_->Sample();
  double max = max_->Sample();

  if (min >= max) {
    throw InvalidArgument("Sampled min value is more than sampled max"
                          " for Uniform distribution.");
  }

  return Random::UniformRealGenerator(min, max);
}

void NormalDeviate::Validate() {
  if (sigma_->Mean() <= 0) {
    throw InvalidArgument("Standard deviation cannot be negative or zero.");
  }
}

double NormalDeviate::Sample() {
  double sigma = sigma_->Sample();
  if (sigma <= 0) {
    throw InvalidArgument("Sampled standard deviation is negative or zero.");
  }
  return Random::NormalGenerator(mean_->Sample(), sigma);
}

void LogNormalDeviate::Validate() {
  if (level_->Mean() != 0.95) {
    throw InvalidArgument("The confidence level is expected to be 0.95.");
  } else if (ef_->Mean() <= 0) {
    throw InvalidArgument("The Error Factor for Log-Normal distribution"
                          " cannot be negative or zero.");
  } else if (mean_->Mean() <= 0) {
    throw InvalidArgument("The mean of Log-Normal distribution cannot be"
                          " negative or zero.");
  }
}

double LogNormalDeviate::Sample() {
  double mean = mean_->Sample();
  double ef = ef_->Sample();
  if (ef <= 0) {
    throw InvalidArgument("The Sampled Error Factor for Log-Normal"
                          " distribution cannot be negative or zero.");
  } else if (mean <= 0) {
    throw InvalidArgument("The sampled mean of Log-Normal distribution"
                          " cannot be negative or zero.");
  }
  double sigma = std::log(ef) / 1.645;
  double mu = std::log(mean) - std::pow(sigma, 2) / 2;
  return Random::LogNormalGenerator(mu, sigma);
}

void GammaDeviate::Validate() {
  if (k_->Mean() <= 0) {
    throw InvalidArgument("The k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta_->Mean() <= 0) {
    throw InvalidArgument("The theta scale parameter for Gamma distribution"
                          " cannot be negative or zero.");
  }
}

double GammaDeviate::Sample() {
  double k = k_->Sample();
  double theta = theta_->Sample();
  if (k <= 0) {
    throw InvalidArgument("Sampled k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta <= 0) {
    throw InvalidArgument("Sampled theta scale parameter for Gamma "
                          "distribution cannot be negative or zero.");
  }
  return Random::GammaGenerator(k, theta);
}

void BetaDeviate::Validate() {
  if (alpha_->Mean() <= 0) {
    throw InvalidArgument("The alpha shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (beta_->Mean() <= 0) {
    throw InvalidArgument("The beta shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  }
}

double BetaDeviate::Sample() {
  double alpha = alpha_->Sample();
  double beta = beta_->Sample();
  if (alpha <= 0) {
    throw InvalidArgument("Sampled alpha shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (beta <= 0) {
    throw InvalidArgument("Sampled beta shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  }
  return Random::BetaGenerator(alpha, beta);
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
  std::vector<double> b;
  b.push_back(0);  // The initial point.
  std::vector<double> w;
  for (int i = 0; i < boundaries_.size(); ++i) {
    b.push_back(boundaries_[i]->Sample());
    w.push_back(weights_[i]->Sample());
  }
  CheckBoundaries(b);
  CheckWeights(w);
  return Random::HistogramGenerator(w, b);
}

void Histogram::CheckBoundaries(const std::vector<ExpressionPtr>& boundaries) {
  if (boundaries[0]->Mean() <= 0) {
    throw InvalidArgument("Histogram lower boundary must be positive.");
  }
  for (int i = 1; i < boundaries.size(); ++i) {
    if (boundaries[i-1]->Mean() >= boundaries[i]->Mean()) {
      throw InvalidArgument("Histogram upper boundaries are not strictly"
                            " increasing.");
    }
  }
}

void Histogram::CheckBoundaries(const std::vector<double>& boundaries) {
  if (boundaries[1] <= 0) {
    throw InvalidArgument("Histogram sampled lower boundary must be positive.");
  }
  for (int i = 1; i < boundaries.size(); ++i) {
    if (boundaries[i-1] >= boundaries[i]) {
      throw InvalidArgument("Histogram sampled upper boundaries are"
                            " not strictly increasing.");
    }
  }
}

void Histogram::CheckWeights(const std::vector<ExpressionPtr>& weights) {
  for (int i = 0; i < weights.size(); ++i) {
    if (weights[i]->Mean() < 0) {
      throw InvalidArgument("Histogram weights are negative.");
    }
  }
}

void Histogram::CheckWeights(const std::vector<double>& weights) {
  for (int i = 0; i < weights.size(); ++i) {
    if (weights[i] < 0) {
      throw InvalidArgument("Histogram sampled weights are negative.");
    }
  }
}

}  // namespace scram
