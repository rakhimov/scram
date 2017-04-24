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

#include <functional>
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

/// Validation specialization to check division by 0.
template <>
void ValidateExpression<std::divides<>>(const std::vector<Expression*>& args);

using Add = NaryExpression<std::plus<>, -1>;  ///< Sum operation.
using Sub = NaryExpression<std::minus<>, -1>;  ///< Subtraction from the first.
using Mul = NaryExpression<std::multiplies<>, -1>;  ///< Product.
using Div = NaryExpression<std::divides<>, -1>;  ///< Division of the first.

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_ARITHMETIC_H_
