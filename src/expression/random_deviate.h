/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// A collection of deviate expressions with random distributions
/// sampled at run-time.

#pragma once

#include <memory>
#include <random>
#include <vector>

#include <boost/range/iterator_range.hpp>

#include "src/expression.h"

namespace scram::mef {

/// Abstract base class for all deviate expressions.
/// These expressions provide quantification for uncertainty and sensitivity.
///
/// @note Only single RNG is embedded for convenience.
///       All the distributions share this RNG.
///       This is not suitable for parallelized simulations!!!
///
/// @todo Parametrize with RNG (requires mef::Expression interface change).
class RandomDeviate : public Expression {
 public:
  using Expression::Expression;

  bool IsDeviate() noexcept override { return true; }

  /// Sets the seed of the underlying random number generator.
  ///
  /// @param[in] seed  The seed for RNGs.
  ///
  /// @note This is static! Used by all the deriving deviates.
  static void seed(unsigned seed) noexcept { rng_.seed(seed); }

 protected:
  /// @returns RNG to be used by derived classes.
  std::mt19937& rng() { return rng_; }

 private:
  static std::mt19937 rng_;  ///< The random number generator.
};

/// Uniform distribution.
class UniformDeviate : public RandomDeviate {
 public:
  /// Setup for uniform distribution.
  ///
  /// @param[in] min  Minimum value of the distribution.
  /// @param[in] max  Maximum value of the distribution.
  UniformDeviate(Expression* min, Expression* max);

  /// @throws ValidityError  The min value is more or equal to max value.
  void Validate() const override;

  double value() noexcept override { return (min_.value() + max_.value()) / 2; }
  Interval interval() noexcept override {
    return Interval::closed(min_.value(), max_.value());
  }

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
  NormalDeviate(Expression* mean, Expression* sigma);

  /// @throws DomainError  The sigma is negative or zero.
  void Validate() const override;

  double value() noexcept override { return mean_.value(); }
  /// @returns ~99.9% confidence interval.
  Interval interval() noexcept override {
    double mean = mean_.value();
    double delta = 6 * sigma_.value();
    return Interval::closed(mean - delta, mean + delta);
  }

 private:
  double DoSample() noexcept override;

  Expression& mean_;  ///< Mean value of normal distribution.
  Expression& sigma_;  ///< Standard deviation of normal distribution.
};

/// Log-normal distribution.
class LognormalDeviate : public RandomDeviate {
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
  LognormalDeviate(Expression* mean, Expression* ef, Expression* level);

  /// The parametrization with underlying normal distribution parameters.
  ///
  /// @param[in] mu  The mean of the normal distribution.
  /// @param[in] sigma  The standard deviation of the normal distribution.
  LognormalDeviate(Expression* mu, Expression* sigma);

  void Validate() const override { flavor_->Validate(); };
  double value() noexcept override { return flavor_->mean(); }
  /// The high is 99.9 percentile estimate.
  Interval interval() noexcept override;

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
    /// @copydoc LognormalDeviate::LognormalDeviate
    Logarithmic(Expression* mean, Expression* ef, Expression* level)
        : mean_(*mean), ef_(*ef), level_(*level) {}
    double scale() noexcept override;
    double location() noexcept override;
    double mean() noexcept override { return mean_.value(); }
    /// @throws DomainError  (mean <= 0) or (ef <= 0) or invalid level.
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
    Normal(Expression* mu, Expression* sigma) : mu_(*mu), sigma_(*sigma) {}
    double scale() noexcept override { return sigma_.value(); }
    double location() noexcept override { return mu_.value(); }
    double mean() noexcept override;
    /// @throws DomainError  (sigma <= 0).
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
  GammaDeviate(Expression* k, Expression* theta);

  /// @throws DomainError  (k <= 0) or (theta <= 0)
  void Validate() const override;

  double value() noexcept override { return k_.value() * theta_.value(); }
  /// The high is 99 percentile.
  Interval interval() noexcept override;

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
  BetaDeviate(Expression* alpha, Expression* beta);

  /// @throws DomainError  (alpha <= 0) or (beta <= 0)
  void Validate() const override;

  double value() noexcept override {
    double alpha_mean = alpha_.value();
    return alpha_mean / (alpha_mean + beta_.value());
  }

  /// @returns 99 percentile.
  Interval interval() noexcept override;

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
  /// @throws ValidityError  The boundaries container size is not equal to
  ///                        weights container size + 1.
  Histogram(std::vector<Expression*> boundaries,
            std::vector<Expression*> weights);

  /// @throws ValidityError  The boundaries are not strictly increasing,
  ///                        or weights are negative.
  void Validate() const override;

  double value() noexcept override;
  Interval interval() noexcept override {
    return Interval::closed((*boundaries_.begin())->value(),
                            (*std::prev(boundaries_.end()))->value());
  }

 private:
  /// Access to args.
  using IteratorRange =
      boost::iterator_range<std::vector<Expression*>::const_iterator>;

  double DoSample() noexcept override;

  IteratorRange boundaries_;  ///< Boundaries of the intervals.
  IteratorRange weights_;  ///< Weights of the intervals.
};

}  // namespace scram::mef
