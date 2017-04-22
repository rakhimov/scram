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

/// @file arithmetic.h
/// A collection of arithmetic expressions.

#ifndef SCRAM_SRC_EXPRESSION_ARITHMETIC_H_
#define SCRAM_SRC_EXPRESSION_ARITHMETIC_H_

#include <vector>

#include "src/expression.h"

namespace scram {
namespace mef {

/// This class for negation of numerical value or another expression.
class Neg : public Expression {
 public:
  /// Construct a new expression
  /// that negates a given argument expression.
  ///
  /// @param[in] expression  The expression to be negated.
  explicit Neg(Expression* expression);

  double Mean() noexcept override { return -expression_.Mean(); }
  Interval interval() noexcept override {
    Interval arg_interval = expression_.interval();
    return {-arg_interval.upper(), -arg_interval.lower(),
            ReverseBounds(arg_interval)};
  }

 private:
  double DoSample() noexcept override { return -expression_.Sample(); }

  Expression& expression_;  ///< Expression that is used for negation.
};

/// Base class for expressions that require 2 or more arguments.
class BinaryExpression : public Expression {
 public:
  /// Checks the number of provided arguments upon initialization.
  ///
  /// @param[in] args  Arguments of this expression.
  ///
  /// @throws InvalidArgument  The number of arguments is fewer than 2.
  explicit BinaryExpression(std::vector<Expression*> args);
};

/// This expression adds all the given expressions' values.
class Add : public BinaryExpression {
 public:
  using BinaryExpression::BinaryExpression;

  double Mean() noexcept override { return Compute(&Expression::Mean); }
  Interval interval() noexcept override;

 private:
  double DoSample() noexcept override { return Compute(&Expression::Sample); }

  /// Adds all argument expression values.
  ///
  /// @param[in] value  The getter function for the arg expression value.
  ///
  /// @returns The sum of the expression values.
  double Compute(double (Expression::*value)()) {
    double result = 0;
    for (Expression* arg : Expression::args())
      result += ((*arg).*value)();
    return result;
  }
};

/// This expression performs subtraction operation.
/// First expression minus the rest of the given expressions' values.
class Sub : public BinaryExpression {
 public:
  using BinaryExpression::BinaryExpression;

  double Mean() noexcept override { return Compute(&Expression::Mean); }
  Interval interval() noexcept override;

 private:
  double DoSample() noexcept override { return Compute(&Expression::Sample); }

  /// Performs the subtraction of all argument expression values.
  ///
  /// @param[in] value  The getter function for the arg expression value.
  ///
  /// @returns first_value - sum(rest_value).
  double Compute(double (Expression::*value)()) {
    auto it = Expression::args().begin();
    double result = ((**it).*value)();
    for (++it; it != Expression::args().end(); ++it) {
      result -= ((**it).*value)();
    }
    return result;
  }
};

/// This expression performs multiplication operation.
class Mul : public BinaryExpression {
 public:
  using BinaryExpression::BinaryExpression;

  double Mean() noexcept override;
  Interval interval() noexcept override;

 private:
  double DoSample() noexcept override;
};

/// This expression performs division operation.
/// The expression divides the first given argument by
/// the rest of argument expressions.
class Div : public BinaryExpression {
 public:
  using BinaryExpression::BinaryExpression;

  /// @throws InvalidArgument  Division by 0.
  void Validate() const override;

  double Mean() noexcept override;
  Interval interval() noexcept override;

 private:
  double DoSample() noexcept override;
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_ARITHMETIC_H_
