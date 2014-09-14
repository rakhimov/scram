/// @file random.cc
/// Implementation for various Rngs.
#include "random.h"

#include <boost/random.hpp>

typedef boost::mt19937 RandomGenerator;

namespace scram {

Random::Random(int seed) {
  rng_.seed(static_cast<unsigned>(seed));
}

double Random::UniformRealGenerator(double min, double max) {
  typedef boost::uniform_real<double> UniformDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      UniformDistribution> UniformGenerator;

  UniformDistribution uniform_dist(min, max);

  // Create a generator
  UniformGenerator generator(rng_, uniform_dist);

  return generator();
}

double Random::TriangularGenerator(double lower, double mode, double upper) {
  typedef boost::triangle_distribution<double> TriangularDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      TriangularDistribution> TriangularGenerator;

  // Choose Triangle Distribution
  TriangularDistribution triangular_distribution(lower, mode, upper);

  // Create a generator
  TriangularGenerator generator(rng_, triangular_distribution);

  return generator();
}

double Random::NormalGenerator(double mean, double sigma) {
  typedef boost::normal_distribution<double> NormalDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      NormalDistribution> GaussianGenerator;

  // Choose Normal Distribution
  NormalDistribution gaussian_dist(mean, sigma);

  // Create a Gaussian Random Number generator
  // by binding with previously defined
  // normal distribution object
  GaussianGenerator generator(rng_, gaussian_dist);

  return generator();
}

double Random::LogNormalGenerator(double mean, double sigma) {
  typedef boost::lognormal_distribution<double> LogNormalDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      LogNormalDistribution> LogNormalGenerator;

  LogNormalDistribution lognorm_dist(mean, sigma);

  // Create a generator
  LogNormalGenerator generator(rng_, lognorm_dist);

  return generator();
}

double Random::PoissonGenerator(double mean) {
  typedef boost::poisson_distribution<int, double> PoissonDistribution;
  typedef boost::variate_generator<RandomGenerator&, \
      PoissonDistribution> PoissonGenerator;

  PoissonDistribution poisson_distribution(mean);

  // Create a generator
  PoissonGenerator generator(rng_, poisson_distribution);

  return generator();
}

}  // namespace scram
