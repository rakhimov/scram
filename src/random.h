/// @file random.h
/// Contains helpers for randomness simulations.
#ifndef SCRAM_RANDOM_H_
#define SCRAM_RANDOM_H_

namespace scram {

class Random {
 public:
  virtual ~Random() {}

  /// Rng from a normal distribution
  double NormalGenerator(double mean, double sigma, int seed);

  /// Rng from a triangular distribution
  double TriangularGenerator(double lower, double mode, double upper,
                             int seed);

  /// Rng from uniform distribution. 0 to 1 is the default.
  double UniformRealGenerator(double min, double max, int seed);

  /// Rng from Poisson distribution
  double PoissonGenerator(double mean, int seed);

  /// Rng from lognormal distribution
  double LogNormalGenerator(double mean, double sigma, int seed);
};

}  // namespace scram

#endif  // SCRAM_RANDOM_H_
