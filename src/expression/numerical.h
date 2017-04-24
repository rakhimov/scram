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

/// @file numerical.h
/// A collection of numerical expressions.
/// @note The PI value is located in constant expressions.

#ifndef SCRAM_SRC_EXPRESSION_NUMERICAL_H_
#define SCRAM_SRC_EXPRESSION_NUMERICAL_H_

#include <cmath>

#include <functional>
#include <vector>

#include "constant.h"
#include "src/expression.h"

namespace scram {
namespace mef {

/// Creates a functor out of function pointer to common cmath functions.
template <double (*F)(double)>
struct Functor {
  /// Forwards the call to the wrapped function.
  double operator()(double arg) { return F(arg); }
};

/// Expression with a functor wrapping a function.
template <double (*F)(double)>
using FunctorExpression = NaryExpression<Functor<F>, 1>;

/// Validation specialization for math functions.
/// @{
template <>
void ValidateExpression<std::divides<>>(const std::vector<Expression*>& args);
template <>
void ValidateExpression<Functor<&std::acos>>(
    const std::vector<Expression*>& args);
/// @}

/// Interval specialization for math functions.
/// @{
template <>
inline Interval GetInterval<Functor<&std::acos>>(Expression* /*arg*/) {
  return Interval::closed(0, ConstantExpression::kPi.value());
}
/// @}

using Neg = NaryExpression<std::negate<>, 1>;  ///< Negation.
using Add = NaryExpression<std::plus<>, -1>;  ///< Sum operation.
using Sub = NaryExpression<std::minus<>, -1>;  ///< Subtraction from the first.
using Mul = NaryExpression<std::multiplies<>, -1>;  ///< Product.
using Div = NaryExpression<std::divides<>, -1>;  ///< Division of the first.
using Abs = FunctorExpression<&std::abs>;  ///< The absolute value.
using Acos = FunctorExpression<&std::acos>;  ///< Arc cosine.

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_NUMERICAL_H_
