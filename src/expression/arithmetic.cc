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

/// @file arithmetic.cc
/// Implementation of various arithmetic expressions.

#include "arithmetic.h"

#include <algorithm>

#include "src/error.h"

namespace scram {
namespace mef {

Neg::Neg(const ExpressionPtr& expression)
    : Expression({expression}),
      expression_(*expression) {}

BinaryExpression::BinaryExpression(std::vector<ExpressionPtr> args)
    : Expression(std::move(args)) {
  if (Expression::args().size() < 2)
    throw InvalidArgument("Expression requires 2 or more arguments.");
}

double Mul::Mean() noexcept {
  double mean = 1;
  for (const ExpressionPtr& arg : Expression::args())
    mean *= arg->Mean();
  return mean;
}

double Mul::DoSample() noexcept {
  double result = 1;
  for (const ExpressionPtr& arg : Expression::args())
    result *= arg->Sample();
  return result;
}

double Mul::GetExtremum(bool maximum) noexcept {
  double max_val = 1;  // Maximum possible product.
  double min_val = 1;  // Minimum possible product.
  for (const ExpressionPtr& arg : Expression::args()) {
    double mult_max = arg->Max();
    double mult_min = arg->Min();
    double max_max = max_val * mult_max;
    double max_min = max_val * mult_min;
    double min_max = min_val * mult_max;
    double min_min = min_val * mult_min;
    max_val = std::max({max_max, max_min, min_max, min_min});
    min_val = std::min({max_max, max_min, min_max, min_min});
  }
  return maximum ? max_val : min_val;
}

void Div::Validate() const {
  auto it = Expression::args().begin();
  for (++it; it != Expression::args().end(); ++it) {
    const auto& expr = *it;
    if (!expr->Mean() || !expr->Max() || !expr->Min())
      throw InvalidArgument("Division by 0.");
  }
}

double Div::Mean() noexcept {
  auto it = Expression::args().begin();
  double mean = (*it)->Mean();
  for (++it; it != Expression::args().end(); ++it) {
    mean /= (*it)->Mean();
  }
  return mean;
}

double Div::DoSample() noexcept {
  auto it = Expression::args().begin();
  double result = (*it)->Sample();
  for (++it; it != Expression::args().end(); ++it)
    result /= (*it)->Sample();
  return result;
}

double Div::GetExtremum(bool maximum) noexcept {
  auto it = Expression::args().begin();
  double max_value = (*it)->Max();  // Maximum possible result.
  double min_value = (*it)->Min();  // Minimum possible result.
  for (++it; it != Expression::args().end(); ++it) {
    double div_max = (*it)->Max();
    double div_min = (*it)->Min();
    double max_max = max_value / div_max;
    double max_min = max_value / div_min;
    double min_max = min_value / div_max;
    double min_min = min_value / div_min;
    max_value = std::max({max_max, max_min, min_max, min_min});
    min_value = std::min({max_max, max_min, min_max, min_min});
  }
  return maximum ? max_value : min_value;
}

}  // namespace mef
}  // namespace scram
