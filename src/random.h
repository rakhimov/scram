/// @file random.h
/// Contains helpers for randomness simulations.
#ifndef SCRAM_RANDOM_H_
#define SCRAM_RANDOM_H_

#include <boost/random.hpp>

namespace scram {

/// @class Random
/// This class contains generators for various random distributions.
/// The values passed to the member functions are asserted to be in the
/// correct form. In other words, the user should make sure that the passed
/// parameters are valid. For example, variance cannot be negative.
class Random {
 public:
  /// The constructor for Random distibutions.
  /// @param[in] seed The seed for RNGs.
  explicit Random(int seed);

  ~Random() {}

  /// Rng from uniform distribution.
  /// @param[in] min Lower bound.
  /// @param[in] max Upper bound.
  /// @returns A sampled value.
  double UniformRealGenerator(double min, double max);

  /// Rng from a triangular distribution
  /// @param[in] lower Lower bound.
  /// @param[in] mode The peak of the distribution.
  /// @param[in] upper Upper bound.
  /// @returns A sampled value.
  double TriangularGenerator(double lower, double mode, double upper);

  /// Rng from a normal distribution
  /// @param[in] mean The mean of the distribution.
  /// @param[in] sigma The variance of the distribution.
  /// @returns A sampled value.
  double NormalGenerator(double mean, double sigma);

  /// Rng from lognormal distribution
  /// @param[in] mean The mean of the distribution.
  /// @param[in] sigma The variance of the distribution.
  /// @returns A sampled value.
  double LogNormalGenerator(double mean, double sigma);

  /// Rng from Gamma distribution
  /// @param[in] alpha Positive alpha parameter of the Gamma distribution.
  /// @param[in] beta Positive beta parameter of the Gamma distribution.
  /// @returns A sampled value.
  double GammaGenerator(double alpha, double beta);

  /// Rng from Poisson distribution
  /// @param[in] mean The mean value for Poisson distribution.
  /// @returns A sampled value.
  double PoissonGenerator(double mean);

 private:
  /// The random number generator.
  boost::mt19937 rng_;
};

}  // namespace scram

#endif  // SCRAM_RANDOM_H_
