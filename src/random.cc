/// @file random.cc
/// Implementation for various Rngs.
#include "random.h"

#include <boost/random.hpp>

namespace scram {

double Random::NormalGenerator(double mean, double sigma, int seed) {
  typedef boost::normal_distribution<double> NormalDistribution;
  typedef boost::mt19937 RandomGenerator;
  typedef boost::variate_generator<RandomGenerator&, \
      NormalDistribution> GaussianGenerator;

  // Initiate Random Number generator with current time
  static RandomGenerator rng(static_cast<unsigned> (seed));

  // Choose Normal Distribution
  NormalDistribution gaussian_dist(mean, sigma);

  // Create a Gaussian Random Number generator
  // by binding with previously defined
  // normal distribution object
  GaussianGenerator generator(rng, gaussian_dist);

  return generator();
}

double Random::TriangularGenerator(double lower, double mode, double upper,
                                   int seed) {
  typedef boost::triangle_distribution<double> TriangularDistribution;
  typedef boost::mt19937 RandomGenerator;
  typedef boost::variate_generator<RandomGenerator&, \
      TriangularDistribution> TriangularGenerator;

  // Initiate Random Number generator with current time
  static RandomGenerator rng(static_cast<unsigned> (seed));

  // Choose Triangle Distribution
  TriangularDistribution triangular_distribution(lower, mode, upper);
  // Create a generator
  TriangularGenerator generator(rng, triangular_distribution);
  return generator();
}

double Random::UniformRealGenerator(double min, double max, int seed) {
  typedef boost::uniform_real<double> UniformDistribution;
  typedef boost::mt19937 RandomGenerator;
  typedef boost::variate_generator<RandomGenerator&, \
      UniformDistribution> UniformGenerator;

  // Initiate Random Number generator with current time
  static RandomGenerator rng(static_cast<unsigned> (seed));

  UniformDistribution uniform_dist(min, max);

  // Create a generator
  UniformGenerator generator(rng, uniform_dist);

  return generator();
}

double Random::PoissonGenerator(double mean, int seed) {
  typedef boost::poisson_distribution<int, double> PoissonDistribution;
  typedef boost::mt19937 RandomGenerator;
  typedef boost::variate_generator<RandomGenerator&, \
      PoissonDistribution> PoissonGenerator;

  // Initiate Random Number generator with current time
  static RandomGenerator rng(static_cast<unsigned> (seed));

  PoissonDistribution poisson_distribution(mean);

  // Create a generator
  PoissonGenerator generator(rng, poisson_distribution);

  return generator();
}

double LogNormalGenerator(double mean, double sigma, int seed) {
  typedef boost::lognormal_distribution<double> LogNormalDistribution;
  typedef boost::mt19937 RandomGenerator;
  typedef boost::variate_generator<RandomGenerator&, \
      LogNormalDistribution> LogNormalGenerator;

  // Initiate Random Number generator with current time
  static RandomGenerator rng(static_cast<unsigned> (seed));

  LogNormalDistribution lognorm_dist(mean, sigma);

  // Create a generator
  LogNormalGenerator generator(rng, lognorm_dist);

  return generator();
}

}  // namespace scram
