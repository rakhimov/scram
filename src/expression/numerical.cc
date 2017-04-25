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

/// @file numerical.cc
/// Implementation of various numerical expressions.

#include "numerical.h"

#include "src/error.h"

namespace scram {
namespace mef {

template <>
void ValidateExpression<std::divides<>>(const std::vector<Expression*>& args) {
  auto it = args.begin();
  for (++it; it != args.end(); ++it) {
    const auto& expr = *it;
    Interval arg_interval = expr->interval();
    if (expr->value() == 0 || Contains(arg_interval, 0))
      throw InvalidArgument("Division by 0.");
  }
}

template <>
void ValidateExpression<Functor<&std::acos>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 1);
  EnsureWithin<InvalidArgument>(args.front(), Interval::closed(-1, 1),
                                "Arc cos");
}

template <>
void ValidateExpression<Functor<&std::asin>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 1);
  EnsureWithin<InvalidArgument>(args.front(), Interval::closed(-1, 1),
                                "Arc sin");
}

template <>
void ValidateExpression<Functor<&std::log>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 1);
  EnsurePositive<InvalidArgument>(args.front(), "Natural Logarithm");
}

template <>
void ValidateExpression<Functor<&std::log10>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 1);
  EnsurePositive<InvalidArgument>(args.front(), "Decimal Logarithm");
}

template <>
void ValidateExpression<std::modulus<int>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 2);
  auto* arg_two = args.back();
  int arg_value = arg_two->value();
  if (arg_value == 0)
    throw InvalidArgument("Modulo second operand must not be 0.");
  Interval interval = arg_two->interval();
  int high = interval.upper();
  int low = interval.lower();
  if (high == 0 || low == 0 || (low < 0 && 0 < high))
    throw InvalidArgument("Modulo second operand sample must not contain 0.");
}

template <>
void ValidateExpression<Bifunctor<&std::pow>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 2);
  auto* arg_one = args.front();
  auto* arg_two = args.back();
  if (arg_one->value() == 0 && arg_two->value() <= 0)
    throw InvalidArgument("0 to power 0 or less is undefined.");
  if (Contains(arg_one->interval(), 0) && !IsPositive(arg_two->interval()))
    throw InvalidArgument("Power expression 'base' sample range contains 0;"
                          "positive exponent is required.");
}

template <>
void ValidateExpression<Functor<&std::sqrt>>(
    const std::vector<Expression*>& args) {
  assert(args.size() == 1);
  EnsureNonNegative<InvalidArgument>(args.front(), "Square root argument");
}

Mean::Mean(std::vector<Expression*> args) : ExpressionFormula(std::move(args)) {
  if (Expression::args().size() < 2)
    throw InvalidArgument("Expression requires 2 or more arguments.");
}

}  // namespace mef
}  // namespace scram
