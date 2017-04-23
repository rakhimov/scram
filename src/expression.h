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

/// @file expression.h
/// Provides the base class for all expressions
/// and units for expression values.

#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <cstdint>

#include <string>
#include <vector>

#include <boost/icl/continuous_interval.hpp>
#include <boost/noncopyable.hpp>

#include "element.h"

namespace scram {
namespace mef {

/// Validation domain interval for expression values.
using Interval = boost::icl::continuous_interval<double>;
/// left_open, open, right_open, closed bounds.
using IntervalBounds = boost::icl::interval_bounds;

/// Returns right and left interval bounds reversed.
inline IntervalBounds ReverseBounds(const Interval& interval) {
  IntervalBounds bound_type = interval.bounds();
  if (bound_type == IntervalBounds::left_open())
    return IntervalBounds::right_open();
  if (bound_type == IntervalBounds::right_open())
    return IntervalBounds::left_open();
  return bound_type;  // Either fully open or closed.
}

/// Checks if a given interval is within the probability domain.
inline bool IsProbability(const Interval& interval) {
  return boost::icl::within(interval, Interval::closed(0, 1));
}

/// Checks if all values in a given interval are positive.
inline bool IsPositive(const Interval& interval) {
  if (interval.lower() > 0)
    return true;
  if (interval.lower() == 0) {
    IntervalBounds bound_type = interval.bounds();
    return bound_type == IntervalBounds::left_open() ||
           bound_type == IntervalBounds::open();
  }
  return false;
}

/// Checks if all values in a given interval are non-negative.
inline bool IsNonNegative(const Interval& interval) {
  return interval.lower() >= 0;
}

/// Abstract base class for all sorts of expressions to describe events.
/// This class also acts like a connector for parameter nodes
/// and may create cycles.
/// Expressions are not expected to be shared
/// except for parameters.
/// In addition, expressions are not expected to be changed
/// after validation phases.
class Expression : private boost::noncopyable {
 public:
  /// Constructor for use by derived classes
  /// to register their arguments.
  ///
  /// @param[in] args  Arguments of this expression.
  explicit Expression(std::vector<Expression*> args = {});

  virtual ~Expression() = default;

  /// @returns A set of arguments of the expression.
  const std::vector<Expression*>& args() const { return args_; }

  /// Validates the expression.
  /// This late validation is due to parameters that are defined late.
  ///
  /// @throws InvalidArgument  The arguments are invalid for setup.
  virtual void Validate() const {}

  /// @returns The mean value of this expression.
  virtual double value() noexcept = 0;

  /// @returns The domain interval for validation purposes only.
  virtual Interval interval() noexcept {
    double value = this->value();
    return Interval::closed(value, value);
  }

  /// Determines if the value of the expression contains deviate expressions.
  /// The default logic is to check arguments with uncertainties for sampling.
  /// Derived expression classes must decide
  /// if they don't have arguments,
  /// or if they are random deviates.
  ///
  /// @returns true if the expression's value deviates from its mean.
  /// @returns false if the expression's value not need sampling.
  ///
  /// @warning Improper registration of arguments
  ///          may yield silent failure.
  virtual bool IsDeviate() noexcept;

  /// @returns A sampled value of this expression.
  double Sample() noexcept;

  /// This routine resets the sampling to get new values.
  /// All the arguments are called to reset themselves.
  /// If this expression was not sampled,
  /// its arguments are not going to get any calls.
  virtual void Reset() noexcept;

 protected:
  /// Registers an additional argument expression.
  ///
  /// @param[in] arg  An argument expression used by this expression.
  void AddArg(Expression* arg) { args_.push_back(arg); }

 private:
  /// Runs sampling of the expression.
  /// Derived concrete classes must provide the calculation.
  ///
  /// @returns A sampled value of this expression.
  virtual double DoSample() noexcept = 0;

  std::vector<Expression*> args_;  ///< Expression's arguments.
  double sampled_value_;  ///< The sampled value.
  bool sampled_;  ///< Indication if the expression is already sampled.
};

/// CRTP for Expressions with a same formula to evaluate and sample.
///
/// @tparam T  The Expression type with Compute function.
template <class T>
class ExpressionFormula : public Expression {
 public:
  using Expression::Expression;

  /// Computes the expression with argument expression default values.
  double value() noexcept final {
    return static_cast<T*>(this)->Compute(
        [](Expression* arg) { return arg->value(); });
  }

 private:
  /// Computes the expression with argument expression sampled values.
  double DoSample() noexcept final {
    return static_cast<T*>(this)->Compute(
        [](Expression* arg) { return arg->Sample(); });
  }
};

/// Ensures that expression can be used for probability ([0, 1]).
///
/// @tparam T  The exception type to throw for invalid probability values.
///
/// @param[in] expression  The expression to be validated.
/// @param[in] description  The addition information for error messages.
/// @param[in] type  The type of probability or fraction for error messages.
///
/// @throws T  The expression is not suited for probability.
template <typename T>
void EnsureProbability(Expression* expression, const std::string& description,
                       const char* type = "probability") {
  double value = expression->value();
  if (value < 0 || value > 1)
    throw T("Invalid " + std::string(type) + " value for " + description);
  if (IsProbability(expression->interval()) == false)
    throw T("Invalid " + std::string(type) + " sample domain for " +
            description);
}

/// Ensures that expression yields positive (> 0) values.
///
/// @tparam T  The exception type to throw for invalid values.
///
/// @param[in] expression  The expression to be validated.
/// @param[in] description  The addition information for error messages.
///
/// @throws T  The expression is not suited for positive values.
template <typename T>
void EnsurePositive(Expression* expression, const std::string& description) {
  if (expression->value() <= 0)
    throw T(description + " value must be positive.");
  if (IsPositive(expression->interval()) == false)
    throw T(description + " sample domain must be positive.");
}

/// Ensures that expression yields non-negative (>= 0) values.
///
/// @tparam T  The exception type to throw for invalid values.
///
/// @param[in] expression  The expression to be validated.
/// @param[in] description  The addition information for error messages.
///
/// @throws T  The expression is not suited for non-negative values.
template <typename T>
void EnsureNonNegative(Expression* expression, const std::string& description) {
  if (expression->value() < 0)
    throw T(description + " value cannot be negative.");
  if (IsNonNegative(expression->interval()) == false)
    throw T(description + " sample domain cannot have negative values.");
}

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
