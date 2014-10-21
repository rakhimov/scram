/// @file random.cc
/// Implementation for various Rngs.
#include "random.h"

#include <cmath>

typedef boost::mt19937 RandomGenerator;

boost::mt19937 scram::Random::rng_;

namespace scram {

void Random::seed(int seed) { Random::rng_.seed(static_cast<unsigned>(seed)); }

double Random::UniformRealGenerator(double min, double max) {
  assert(min < max);
  typedef boost::uniform_real<double> UniformDistribution;
  typedef boost::variate_generator<RandomGenerator&, UniformDistribution>
      UniformGenerator;

  UniformDistribution uniform_dist(min, max);
  UniformGenerator generator(rng_, uniform_dist);

  return generator();
}

double Random::TriangularGenerator(double lower, double mode, double upper) {
  assert(lower < mode);
  assert(mode < upper);
  typedef boost::random::triangle_distribution<double> TriangularDistribution;
  typedef boost::variate_generator<RandomGenerator&, TriangularDistribution>
      TriangularGenerator;

  TriangularDistribution triangular_distribution(lower, mode, upper);
  TriangularGenerator generator(rng_, triangular_distribution);

  return generator();
}

double Random::PiecewiseLinearGenerator(const std::vector<double>& intervals,
                                        const std::vector<double>& weights) {
  typedef boost::random::piecewise_linear_distribution<double> PLDistribution;
  typedef boost::variate_generator<RandomGenerator&, PLDistribution>
      PLGenerator;

  PLDistribution piecewise_linear_dist(intervals.begin(), intervals.end(),
                                       weights.begin());
  PLGenerator generator(rng_, piecewise_linear_dist);

  return generator();
}

double Random::HistogramGenerator(const std::vector<double>& intervals,
                                  const std::vector<double>& weights) {
  typedef boost::random::piecewise_constant_distribution<double>
      HistogramDistribution;
  typedef boost::variate_generator<RandomGenerator&, HistogramDistribution>
      HistogramGenerator;

  HistogramDistribution histogram_dist(intervals.begin(), intervals.end(),
                                       weights.begin());
  HistogramGenerator generator(rng_, histogram_dist);

  return generator();
}

int Random::DiscreteGenerator(const std::vector<double>& weights) {
  typedef boost::random::discrete_distribution<int> DiscreteDistribution;
  typedef boost::variate_generator<RandomGenerator&, DiscreteDistribution>
      DiscreteGenerator;

  DiscreteDistribution discrete_dist(weights.begin(), weights.end());
  DiscreteGenerator generator(rng_, discrete_dist);

  return generator();
}

int Random::BinomialGenerator(int n, double p) {
  typedef boost::random::binomial_distribution<int> BinomialDistribution;
  typedef boost::variate_generator<RandomGenerator&, BinomialDistribution>
      BinomialGenerator;

  BinomialDistribution binomial_dist(n, p);
  BinomialGenerator generator(rng_, binomial_dist);

  return generator();
}

double Random::NormalGenerator(double mean, double sigma) {
  assert(sigma >= 0);
  typedef boost::random::normal_distribution<double> NormalDistribution;
  typedef boost::variate_generator<RandomGenerator&, NormalDistribution>
      GaussianGenerator;

  NormalDistribution gaussian_dist(mean, sigma);
  GaussianGenerator generator(rng_, gaussian_dist);

  return generator();
}

double Random::LogNormalGenerator(double m, double s) {
  assert(s >= 0);
  typedef boost::random::lognormal_distribution<double> LogNormalDistribution;
  typedef boost::variate_generator<RandomGenerator&, LogNormalDistribution>
      LogNormalGenerator;

  LogNormalDistribution lognorm_dist(m, s);
  LogNormalGenerator generator(rng_, lognorm_dist);

  return generator();
}

double Random::GammaGenerator(double k, double theta) {
  assert(k > 0);
  assert(theta > 0);
  typedef boost::random::gamma_distribution<double> GammaDistribution;
  typedef boost::variate_generator<RandomGenerator&, GammaDistribution>
      GammaGenerator;

  GammaDistribution gamma_dist(k);
  GammaGenerator generator(rng_, gamma_dist);

  return theta * generator();
}

double Random::BetaGenerator(double alpha, double beta) {
  assert(alpha > 0);
  assert(beta > 0);
  typedef boost::random::gamma_distribution<double> GammaDistribution;
  typedef boost::variate_generator<RandomGenerator&, GammaDistribution>
      GammaGenerator;

  GammaDistribution gamma_dist_x(alpha);
  GammaGenerator generator_x(rng_, gamma_dist_x);

  GammaDistribution gamma_dist_y(beta);
  GammaGenerator generator_y(rng_, gamma_dist_y);

  double x = generator_x();
  double y = generator_y();

  return x / (x + y);
}

double Random::WeibullGenerator(double k, double lambda) {
  assert(k > 0);
  assert(lambda > 0);
  typedef boost::random::weibull_distribution<double> WeibullDistribution;
  typedef boost::variate_generator<RandomGenerator&, WeibullDistribution>
      WeibullGenerator;

  WeibullDistribution weibull_dist(k, lambda);
  WeibullGenerator generator(rng_, weibull_dist);

  return generator();
}

double Random::ExponentialGenerator(double lambda) {
  assert(lambda > 0);
  typedef boost::random::exponential_distribution<double>
      ExponentialDistribution;
  typedef boost::variate_generator<RandomGenerator&, ExponentialDistribution>
      ExponentialGenerator;

  ExponentialDistribution exponential_dist(lambda);
  ExponentialGenerator generator(rng_, exponential_dist);

  return generator();
}

double Random::PoissonGenerator(double mean) {
  assert(mean > 0);
  typedef boost::random::poisson_distribution<int, double> PoissonDistribution;
  typedef boost::variate_generator<RandomGenerator&, PoissonDistribution>
      PoissonGenerator;

  PoissonDistribution poisson_distribution(mean);
  PoissonGenerator generator(rng_, poisson_distribution);

  return generator();
}

double Random::LogUniformGenerator(double min, double max) {
  return std::exp(Random::UniformRealGenerator(min, max));
}

double Random::LogTriangularGenerator(double lower, double mode,
                                      double upper) {
  return std::exp(Random::TriangularGenerator(lower, mode, upper));
}

}  // namespace scram
