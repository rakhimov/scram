/// @file random.cc
/// Implementation for various Rngs.
#include "random.h"

#include <boost/version.hpp>

typedef boost::mt19937 RandomGenerator;

namespace scram {

Random::Random(int seed) {
  rng_.seed(static_cast<unsigned>(seed));
}

double Random::UniformRealGenerator(double min, double max) {
  assert(min < max);
  typedef boost::uniform_real<double> UniformDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      UniformDistribution> UniformGenerator;

  UniformDistribution uniform_dist(min, max);
  UniformGenerator generator(rng_, uniform_dist);

  return generator();
}

double Random::TriangularGenerator(double lower, double mode, double upper) {
  assert(lower < mode);
  assert(mode < upper);
  typedef boost::triangle_distribution<double> TriangularDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      TriangularDistribution> TriangularGenerator;

  TriangularDistribution triangular_distribution(lower, mode, upper);
  TriangularGenerator generator(rng_, triangular_distribution);

  return generator();
}

double Random::NormalGenerator(double mean, double sigma) {
  assert(sigma >= 0);
  typedef boost::normal_distribution<double> NormalDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      NormalDistribution> GaussianGenerator;

  NormalDistribution gaussian_dist(mean, sigma);
  GaussianGenerator generator(rng_, gaussian_dist);

  return generator();
}

double Random::LogNormalGenerator(double mean, double sigma) {
  assert(sigma >= 0);
  typedef boost::lognormal_distribution<double> LogNormalDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      LogNormalDistribution> LogNormalGenerator;

  LogNormalDistribution lognorm_dist(mean, sigma);
  LogNormalGenerator generator(rng_, lognorm_dist);

  return generator();
}

double Random::GammaGenerator(double k, double theta) {
  assert(k > 0);
  assert(theta > 0);
  typedef boost::gamma_distribution<double> GammaDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      GammaDistribution> GammaGenerator;

  GammaDistribution gamma_dist(k);
  GammaGenerator generator(rng_, gamma_dist);

  return theta * generator();
}

double Random::BetaGenerator(double alpha, double beta) {
  assert(alpha > 0);
  assert(beta > 0);
  typedef boost::gamma_distribution<double> GammaDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      GammaDistribution> GammaGenerator;

  GammaDistribution gamma_dist_x(alpha);
  GammaGenerator generator_x(rng_, gamma_dist_x);

  GammaDistribution gamma_dist_y(beta);
  GammaGenerator generator_y(rng_, gamma_dist_y);

  double x = generator_x();
  double y = generator_y();

  return x / (x + y);
}

double Random::PoissonGenerator(double mean) {
  assert(mean > 0);
  typedef boost::poisson_distribution<int, double> PoissonDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      PoissonDistribution> PoissonGenerator;

  PoissonDistribution poisson_distribution(mean);
  PoissonGenerator generator(rng_, poisson_distribution);

  return generator();
}

}  // namespace scram
