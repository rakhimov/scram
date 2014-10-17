/// @file expression.cc
/// Implementation of various expressions for basic event probability
/// description.
#include "expression.h"

#include <algorithm>

#include "error.h"

typedef boost::shared_ptr<scram::Parameter> ParameterPtr;

namespace scram {

void Parameter::CheckCyclicity() {
  std::vector<std::string> path;
  CheckCyclicity(&path);
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

NormalDeviate::NormalDeviate(const ExpressionPtr& mean,
                             const ExpressionPtr& sigma)
    : mean_(mean) {
  sigma_ = sigma;
  if (sigma_->Mean() <= 0) {
    throw InvalidArgument("Standard deviation cannot be negative or zero.");
  }
}

double NormalDeviate::Sample() {
  double sigma = sigma_->Sample();
  if (sigma <= 0) {
    throw InvalidArgument("Sampled standard deviation is negative or zero.");
  }
}

LogNormalDeviate::LogNormalDeviate(const ExpressionPtr& mean,
                                   const ExpressionPtr& ef,
                                   const ExpressionPtr& level) {
  if (level->Mean() != 0.95) {
    throw InvalidArgument("The confidence level is expected to be 0.95.");
  } else if (ef->Mean() <= 0) {
    throw InvalidArgument("The Error Factor for Log-Normal distribution"
                          " cannot be negative or zero.");
  } else if (mean->Mean() <= 0) {
    throw InvalidArgument("The mean of Log-Normal distribution cannot be"
                          " negative or zero.");
  }
  mean_ = mean;
  ef_ = ef;
  level_ = level;
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

GammaDeviate::GammaDeviate(const ExpressionPtr& k,
                           const ExpressionPtr& theta) {
  if (k->Mean() <= 0) {
    throw InvalidArgument("The k shape parameter for Gamma distribution"
                          " cannot be negative or zero.");
  } else if (theta->Mean() <= 0) {
    throw InvalidArgument("The theta scale parameter for Gamma distribution"
                          " cannot be negative or zero.");
  }
  k_ = k;
  theta_ = theta;
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

BetaDeviate::BetaDeviate(const ExpressionPtr& alpha,
                         const ExpressionPtr& beta) {
  if (alpha->Mean() <= 0) {
    throw InvalidArgument("The alpha shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  } else if (beta->Mean() <= 0) {
    throw InvalidArgument("The beta shape parameter for Beta distribution"
                          " cannot be negative or zero.");
  }
  alpha_ = alpha;
  beta_ = beta;
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
  CheckBoundaries(boundaries);
  CheckWeights(weights);

  boundaries_ = boundaries;
  weights_ = weights;

  if (weights_.size() != boundaries_.size()) {
    throw InvalidArgument("The number of weights is not equal to the number"
                          " of boundaries.");
  }
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
  for (int i = 1; i < boundaries.size(); ++i) {
    if (boundaries[i]->Mean() < boundaries[i - 1]->Mean()) {
      throw InvalidArgument("Histogram upper boundaries are not strictly"
                            " increasing.");
    }
  }
}

void Histogram::CheckBoundaries(const std::vector<double>& boundaries) {
  for (int i = 1; i < boundaries.size(); ++i) {
    if (boundaries[i] < boundaries[i - 1]) {
      throw InvalidArgument("Histogram sampled upper boundaries are"
                            " not strictly increasing.");
    }
  }
}

void Histogram::CheckWeights(const std::vector<ExpressionPtr>& weights) {
  for (int i = 1; i < weights.size(); ++i) {
    if (weights[i]->Mean() < 0) {
      throw InvalidArgument("Histogram weights are negative.");
    }
  }
}

void Histogram::CheckWeights(const std::vector<double>& weights) {
  for (int i = 1; i < weights.size(); ++i) {
    if (weights[i] < 0) {
      throw InvalidArgument("Histogram sampled weights are negative.");
    }
  }
}

}  // namespace scram
