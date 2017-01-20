/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file random_deviate.h
/// A collection of deviate expressions with random distributions
/// sampled at run-time.

#ifndef SCRAM_SRC_EXPRESSION_RANDOM_DEVIATE_H_
#define SCRAM_SRC_EXPRESSION_RANDOM_DEVIATE_H_

#include <vector>

#include <boost/range/iterator_range.hpp>

#include "src/expression.h"

namespace scram {
namespace mef {

/// Abstract base class for all deviate expressions.
/// These expressions provide quantification for uncertainty and sensitivity.
class RandomDeviate : public Expression {
 public:
  using Expression::Expression;

  bool IsDeviate() noexcept override { return true; }
};

/// Uniform distribution.
class UniformDeviate : public RandomDeviate {
 public:
  /// Setup for uniform distribution.
  ///
  /// @param[in] min  Minimum value of the distribution.
  /// @param[in] max  Maximum value of the distribution.
  UniformDeviate(const ExpressionPtr& min, const ExpressionPtr& max);

  /// @throws InvalidArgument  The min value is more or equal to max value.
  void Validate() const override;

  double Mean() noexcept override { return (min_.Mean() + max_.Mean()) / 2; }
  double Max() noexcept override { return max_.Max(); }
  double Min() noexcept override { return min_.Min(); }

 private:
  double DoSample() noexcept override;

  Expression& min_;  ///< Minimum value of the distribution.
  Expression& max_;  ///< Maximum value of the distribution.
};

/// Normal distribution.
class NormalDeviate : public RandomDeviate {
 public:
  /// Setup for normal distribution.
  ///
  /// @param[in] mean  The mean of the distribution.
  /// @param[in] sigma  The standard deviation of the distribution.
  NormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& sigma);

  /// @throws InvalidArgument  The sigma is negative or zero.
  void Validate() const override;

  double Mean() noexcept override { return mean_.Mean(); }

  /// @returns ~99.9% percentile value.
  ///
  /// @warning This is only an approximation of the maximum value.
  double Max() noexcept override { return mean_.Max() + 6 * sigma_.Max(); }

  /// @returns Less than 0.1% percentile value.
  ///
  /// @warning This is only an approximation.
  double Min() noexcept override { return mean_.Min() - 6 * sigma_.Max(); }

 private:
  double DoSample() noexcept override;

  Expression& mean_;  ///< Mean value of normal distribution.
  Expression& sigma_;  ///< Standard deviation of normal distribution.
};

/// Log-normal distribution defined by
/// its expected value and error factor of certain confidence level.
class LogNormalDeviate : public RandomDeviate {
 public:
  /// Setup for log-normal distribution.
  ///
  /// @param[in] mean  The mean of the log-normal distribution
  ///                  not the mean of underlying normal distribution,
  ///                  which is parameter mu.
  ///                  mu is the location parameter,
  ///                  sigma is the scale factor.
  ///                  E(x) = exp(mu + sigma^2 / 2)
  /// @param[in] ef  The error factor of the log-normal distribution.
  ///                EF = exp(z_alpha * sigma)
  /// @param[in] level  The confidence level.
  LogNormalDeviate(const ExpressionPtr& mean, const ExpressionPtr& ef,
                   const ExpressionPtr& level);

  /// @throws InvalidArgument  (mean <= 0) or (ef <= 0) or invalid level
  void Validate() const override;

  double Mean() noexcept override { return mean_.Mean(); }

  /// 99.9 percentile estimate.
  double Max() noexcept override;

  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  /// Computes the scale parameter of the distribution.
  ///
  /// @param[in] level  The confidence level.
  /// @param[in] ef  The error factor of the log-normal distribution.
  ///
  /// @returns Scale parameter (sigma) value.
  double ComputeScale(double level, double ef) noexcept;

  /// Computes the location parameter of the distribution.
  ///
  /// @param[in] mean  The mean of the log-normal distribution.
  /// @param[in] sigma  The scale parameter of the distribution.
  ///
  /// @returns Value of location parameter (mu) value.
  double ComputeLocation(double mean, double sigma) noexcept;

  Expression& mean_;  ///< Mean value of the log-normal distribution.
  Expression& ef_;  ///< Error factor of the log-normal distribution.
  Expression& level_;  ///< Confidence level of the log-normal distribution.
};

/// Gamma distribution.
class GammaDeviate : public RandomDeviate {
 public:
  /// Setup for Gamma distribution.
  ///
  /// @param[in] k  Shape parameter of Gamma distribution.
  /// @param[in] theta  Scale parameter of Gamma distribution.
  GammaDeviate(const ExpressionPtr& k, const ExpressionPtr& theta);

  /// @throws InvalidArgument  (k <= 0) or (theta <= 0)
  void Validate() const override;

  double Mean() noexcept override { return k_.Mean() * theta_.Mean(); }

  /// @returns 99 percentile.
  double Max() noexcept override;

  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  Expression& k_;  ///< The shape parameter of the gamma distribution.
  Expression& theta_;  ///< The scale factor of the gamma distribution.
};

/// Beta distribution.
class BetaDeviate : public RandomDeviate {
 public:
  /// Setup for Beta distribution.
  ///
  /// @param[in] alpha  Alpha shape parameter of Gamma distribution.
  /// @param[in] beta  Beta shape parameter of Gamma distribution.
  BetaDeviate(const ExpressionPtr& alpha, const ExpressionPtr& beta);

  /// @throws InvalidArgument  (alpha <= 0) or (beta <= 0)
  void Validate() const override;

  double Mean() noexcept override {
    double alpha_mean = alpha_.Mean();
    return alpha_mean / (alpha_mean + beta_.Mean());
  }

  /// @returns 99 percentile.
  double Max() noexcept override;

  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  Expression& alpha_;  ///< The alpha shape parameter.
  Expression& beta_;  ///< The beta shape parameter.
};

/// Histogram distribution.
class Histogram : public RandomDeviate {
 public:
  /// Histogram distribution setup.
  ///
  /// @param[in] boundaries  The bounds of intervals.
  /// @param[in] weights  The positive weights of intervals
  ///                     restricted by the upper boundaries.
  ///                     Therefore, the number of weights must be
  ///                     equal to the number of intervals.
  ///
  /// @throws InvalidArgument  The boundaries container size is not equal to
  ///                          weights container size + 1.
  Histogram(std::vector<ExpressionPtr> boundaries,
            std::vector<ExpressionPtr> weights);

  /// @throws InvalidArgument  The boundaries are not strictly increasing,
  ///                          or weights are negative.
  void Validate() const override {
    CheckBoundaries();
    CheckWeights();
  }

  double Mean() noexcept override;
  double Max() noexcept override {
    return (*std::prev(boundaries_.end()))->Max();
  }
  double Min() noexcept override { return (*boundaries_.begin())->Min(); }

 private:
  /// Access to args.
  using IteratorRange =
      boost::iterator_range<std::vector<ExpressionPtr>::const_iterator>;

  double DoSample() noexcept override;

  /// Checks if values of boundary expressions are strictly increasing.
  ///
  /// @throws InvalidArgument  The mean values are not strictly increasing.
  void CheckBoundaries() const;

  /// Checks if values of weights are non-negative.
  ///
  /// @throws InvalidArgument  The mean values are negative.
  void CheckWeights() const;

  IteratorRange boundaries_;  ///< Boundaries of the intervals.
  IteratorRange weights_;  ///< Weights of the intervals.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_RANDOM_DEVIATE_H_
