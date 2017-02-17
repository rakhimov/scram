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

#include <memory>
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
  double Max() noexcept override { return max_.Mean(); }
  double Min() noexcept override { return min_.Mean(); }

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
  double Max() noexcept override { return mean_.Mean() + 6 * sigma_.Mean(); }

  /// @returns Less than 0.1% percentile value.
  double Min() noexcept override { return mean_.Mean() - 6 * sigma_.Mean(); }

 private:
  double DoSample() noexcept override;

  Expression& mean_;  ///< Mean value of normal distribution.
  Expression& sigma_;  ///< Standard deviation of normal distribution.
};

/// Log-normal distribution.
class LogNormalDeviate : public RandomDeviate {
 public:
  /// The log-normal deviate parametrization with
  /// its expected value and error factor of certain confidence level.
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

  /// The parametrization with underlying normal distribution parameters.
  ///
  /// @param[in] mu  The mean of the normal distribution.
  /// @param[in] sigma  The standard deviation of the normal distribution.
  LogNormalDeviate(const ExpressionPtr& mu, const ExpressionPtr& sigma);

  void Validate() const override { flavor_->Validate(); };
  double Mean() noexcept override { return flavor_->mean(); }

  /// 99.9 percentile estimate.
  double Max() noexcept override;
  double Min() noexcept override { return 0; }

 private:
  double DoSample() noexcept override;

  /// Support for parametrization differences.
  struct Flavor {
    virtual ~Flavor() = default;
    /// @returns Scale parameter (sigma) value.
    virtual double scale() noexcept = 0;
    /// @returns Value of location parameter (mu) value.
    virtual double location() noexcept = 0;
    /// @returns The mean value of the distribution.
    virtual double mean() noexcept = 0;
    /// @copydoc Expression::Validate
    virtual void Validate() const = 0;
  };

  /// Computation with the log-normal mean and error factor.
  class Logarithmic final : public Flavor {
   public:
    /// @copydoc LogNormalDeviate::LogNormalDeviate
    Logarithmic(const ExpressionPtr& mean, const ExpressionPtr& ef,
                const ExpressionPtr& level)
        : mean_(*mean), ef_(*ef), level_(*level) {}
    double scale() noexcept override;
    double location() noexcept override;
    double mean() noexcept override { return mean_.Mean(); }
    /// @throws InvalidArgument  (mean <= 0) or (ef <= 0) or invalid level.
    void Validate() const override;

   private:
    Expression& mean_;  ///< Mean value of the log-normal distribution.
    Expression& ef_;  ///< Error factor of the log-normal distribution.
    Expression& level_;  ///< Confidence level of the log-normal distribution.
  };

  /// Computation with normal mean and standard distribution.
  class Normal final : public Flavor {
   public:
    /// @param[in] mu  The mean of the normal distribution.
    /// @param[in] sigma  The standard deviation of the normal distribution.
    Normal(const ExpressionPtr& mu, const ExpressionPtr& sigma)
        : mu_(*mu), sigma_(*sigma) {}
    double scale() noexcept override { return sigma_.Mean(); }
    double location() noexcept override { return mu_.Mean(); }
    double mean() noexcept override;
    /// @throws InvalidArgument  (sigma <= 0).
    void Validate() const override;

   private:
    Expression& mu_;  ///< The mean value of the normal distribution.
    Expression& sigma_;  ///< The standard deviation of the normal distribution.
  };

  std::unique_ptr<Flavor> flavor_;  ///< The parametrization flavor.
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
  void Validate() const override;

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

  IteratorRange boundaries_;  ///< Boundaries of the intervals.
  IteratorRange weights_;  ///< Weights of the intervals.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_RANDOM_DEVIATE_H_
