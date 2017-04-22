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

#include "src/error.h"
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

  double value() noexcept override { return -expression_.value(); }
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
template <class T>
class BinaryExpression : public ExpressionFormula<T> {
 public:
  /// Checks the number of provided arguments upon initialization.
  ///
  /// @param[in] args  Arguments of this expression.
  ///
  /// @throws InvalidArgument  The number of arguments is fewer than 2.
  explicit BinaryExpression(std::vector<Expression*> args)
      : ExpressionFormula<T>(std::move(args)) {
    if (Expression::args().size() < 2)
      throw InvalidArgument("Expression requires 2 or more arguments.");
  }
};

/// This expression adds all the given expressions' values.
class Add : public BinaryExpression<Add> {
 public:
  using BinaryExpression::BinaryExpression;

  Interval interval() noexcept override;

  /// Evaluates the sum of the expression values.
  template <typename T>
  double Compute(T&& eval) {
    double result = 0;
    for (Expression* arg : Expression::args())
      result += eval(arg);
    return result;
  }
};

/// This expression performs subtraction operation.
/// First expression minus the rest of the given expressions' values.
class Sub : public BinaryExpression<Sub> {
 public:
  using BinaryExpression::BinaryExpression;

  Interval interval() noexcept override;

  /// Performs the subtraction of all argument expression values.
  template <typename T>
  double Compute(T&& eval) {
    auto it = Expression::args().begin();
    double result = eval(*it);
    for (++it; it != Expression::args().end(); ++it)
      result -= eval(*it);
    return result;
  }
};

/// This expression performs multiplication operation.
class Mul : public BinaryExpression<Mul> {
 public:
  using BinaryExpression::BinaryExpression;

  Interval interval() noexcept override;

  /// Returns the products of argument values.
  template <typename T>
  double Compute(T&& eval) {
    double result = 1;
    for (Expression* arg : Expression::args())
      result *= eval(arg);
    return result;
  }
};

/// This expression performs division operation.
/// The expression divides the first given argument by
/// the rest of argument expressions.
class Div : public BinaryExpression<Div> {
 public:
  using BinaryExpression::BinaryExpression;

  /// @throws InvalidArgument  Division by 0.
  void Validate() const override;

  Interval interval() noexcept override;

  /// Evaluates the division expression.
  template <typename T>
  double Compute(T&& eval) {
    auto it = Expression::args().begin();
    double result = eval(*it);
    for (++it; it != Expression::args().end(); ++it)
      result /= eval(*it);
    return result;
  }
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_ARITHMETIC_H_
