/// @file random.h
/// Contains helpers for randomness simulations.
#ifndef SCRAM_RANDOM_H_
#define SCRAM_RANDOM_H_

namespace scram {

class Random {
 public:
  virtual ~Random() {}

  /// Rng from a normal distribution
  double NormalGenerator(double mean, double sigma, int seed = 123456789);

  /// Rng from a triangular distribution
  double TriangularGenerator(double lower, double mode, double upper,
                             int seed = 123456789);

  /// Rng from uniform distribution. 0 to 1 is the default.
  double UniformRealGenerator(double min = 0, double max = 1,
                              int seed = 123456789);

  /// Rng from Poisson distribution
  double PoissonGenerator(double mean, int seed = 123456789);

  /// Rng from lognormal distribution
  double LogNormalGenerator(double mean, double sigma,
                            int seed = 123456789);
};

}  // namespace scram

#endif  // SCRAM_RANDOM_H_
