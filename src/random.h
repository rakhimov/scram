/// @file random.h
/// Contains helpers for randomness simulations.
#ifndef SCRAM_SRC_RANDOM_H_
#define SCRAM_SRC_RANDOM_H_

#include <vector>

#include <boost/random.hpp>

namespace scram {

/// @class Random
/// This class contains generators for various random distributions.
/// The values passed to the member functions are asserted to be in the
/// correct form. In other words, the user should make sure that the passed
/// parameters are valid. For example, standard deviation cannot be negative.
class Random {
 public:
  /// Sets the seed of the underlying random number generator.
  ///
  /// @param[in] seed The seed for RNGs.
  static void seed(int seed);

  /// RNG from uniform distribution.
  ///
  /// @param[in] min Lower bound.
  /// @param[in] max Upper bound.
  ///
  /// @returns A sampled value.
  static double UniformRealGenerator(double min, double max);

  /// RNG from a triangular distribution.
  ///
  /// @param[in] lower Lower bound.
  /// @param[in] mode The peak of the distribution.
  /// @param[in] upper Upper bound.
  ///
  /// @returns A sampled value.
  static double TriangularGenerator(double lower, double mode, double upper);

  /// RNG from a piecewise linear distribution.
  ///
  /// @param[in] intervals Interval points for the distribution.
  ///                      The values must be strictly increasing.
  /// @param[in] weights Weights at the boundaries. The number of weights
  ///                    must be equal to the number of points.
  ///                    Extra weights are ignored.
  ///
  /// @returns A sampled value.
  static double PiecewiseLinearGenerator(const std::vector<double>& intervals,
                                         const std::vector<double>& weights);

  /// RNG from a histogram distribution.
  ///
  /// @param[in] intervals Interval points for the distribution.
  ///                      The values must be strictly increasing.
  /// @param[in] weights Weights at the boundaries. The number of weights
  ///                    must be one less than the number of points.
  ///                    Extra weights are ignored.
  ///
  /// @returns A sampled value.
  static double HistogramGenerator(const std::vector<double>& intervals,
                                   const std::vector<double>& weights);

  /// RNG from a discrete distribution.
  ///
  /// @param[in] values Discrete values.
  /// @param[in] weights Weights for the corresponding values. The size must
  ///                    be the same as the values vector size.
  ///
  /// @returns A sample Value from the value vector.
  template<class T>
  static inline T DiscreteGenerator(const std::vector<T>& values,
                                    const std::vector<double>& weights) {
    assert(values.size() == weights.size());
    return values[DiscreteGenerator(weights)];
  }

  /// RNG from Binomial distribution.
  ///
  /// @param[in] n Number of trials.
  /// @param[in] p Probability of success.
  ///
  /// @returns The number of successes.
  static int BinomialGenerator(int n, double p);

  /// RNG from a normal distribution.
  ///
  /// @param[in] mean The mean of the distribution.
  /// @param[in] sigma The standard deviation of the distribution.
  ///
  /// @returns A sampled value.
  static double NormalGenerator(double mean, double sigma);

  /// RNG from lognormal distribution.
  ///
  /// @param[in] m The m location parameter of the distribution.
  /// @param[in] s The s scale factor of the distribution.
  ///
  /// @returns A sampled value.
  static double LogNormalGenerator(double m, double s);

  /// RNG from Gamma distribution.
  ///
  /// @param[in] k Shape parameter of Gamma distribution.
  /// @param[in] theta Scale parameter of Gamma distribution.
  ///
  /// @returns A sampled value.
  ///
  /// @note The rate parameter is 1/theta, so for alpha/beta system, pass
  ///       1/beta as a second parameter for this generator.
  static double GammaGenerator(double k, double theta);

  /// RNG from Beta distribution.
  ///
  /// @param[in] alpha Alpha shape parameter of Beta distribution.
  /// @param[in] beta Beta shape parameter of Beta distribution.
  ///
  /// @returns A sampled value.
  static double BetaGenerator(double alpha, double beta);

  /// RNG from Weibull distribution.
  ///
  /// @param[in] k Shape parameter of Weibull distribution.
  /// @param[in] lambda Scale parameter of Weibull distribution.
  ///
  /// @returns A sampled value.
  static double WeibullGenerator(double k, double lambda);

  /// RNG from Exponential distribution.
  ///
  /// @param[in] lambda Rate parameter of Exponential distribution.
  ///
  /// @returns A sampled value.
  static double ExponentialGenerator(double lambda);

  /// RNG from Poisson distribution.
  ///
  /// @param[in] mean The mean value for Poisson distribution.
  ///
  /// @returns A sampled value.
  static double PoissonGenerator(double mean);

  /// RNG from log-uniform distribution.
  ///
  /// @param[in] min Lower bound.
  /// @param[in] max Upper bound.
  ///
  /// @returns A sampled value.
  static double LogUniformGenerator(double min, double max);

  /// RNG from log-triangular distribution.
  ///
  /// @param[in] lower Lower bound.
  /// @param[in] mode The peak of the distribution.
  /// @param[in] upper Upper bound.
  ///
  /// @returns A sampled value.
  static double LogTriangularGenerator(double lower, double mode, double upper);

 private:
  /// RNG from a discrete distribution.
  ///
  /// @param[in] weights Weights for the range [0, n), where n is the size
  ///                    of the vector.
  ///
  /// @returns Integer in the range [0, 1).
  static int DiscreteGenerator(const std::vector<double>& weights);

  static boost::mt19937 rng_;  ///< The random number generator.
};

}  // namespace scram

#endif  // SCRAM_SRC_RANDOM_H_
