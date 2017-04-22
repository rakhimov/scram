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
#include <utility>

namespace scram {
namespace mef {

Neg::Neg(Expression* expression)
    : Expression({expression}),
      expression_(*expression) {}

Interval Add::interval() noexcept {
  double max_value = 0;
  double min_value = 0;
  for (Expression* arg : Expression::args()) {
    Interval arg_interval = arg->interval();
    max_value += arg_interval.upper();
    min_value += arg_interval.lower();
  }
  assert(min_value <= max_value);
  return Interval::closed(min_value, max_value);
}

Interval Sub::interval() noexcept {
  auto it = Expression::args().begin();
  Interval first_arg_interval = (*it)->interval();
  double max_value = first_arg_interval.upper();
  double min_value = first_arg_interval.lower();
  for (++it; it != Expression::args().end(); ++it) {
    Interval next_arg_interval = (*it)->interval();
    max_value -= next_arg_interval.lower();
    min_value -= next_arg_interval.upper();
  }
  assert(min_value <= max_value);
  return Interval::closed(min_value, max_value);
}

Interval Mul::interval() noexcept {
  double max_value = 1;
  double min_value = 1;
  for (Expression* arg : Expression::args()) {
    Interval arg_interval = arg->interval();
    double mult_max = arg_interval.upper();
    double mult_min = arg_interval.lower();
    double max_max = max_value * mult_max;
    double max_min = max_value * mult_min;
    double min_max = min_value * mult_max;
    double min_min = min_value * mult_min;
    std::tie(min_value, max_value) =
        std::minmax({max_max, max_min, min_max, min_min});
  }
  assert(min_value <= max_value);
  return Interval::closed(min_value, max_value);
}

void Div::Validate() const {
  auto it = Expression::args().begin();
  for (++it; it != Expression::args().end(); ++it) {
    const auto& expr = *it;
    Interval arg_interval = expr->interval();
    /// @todo Rethink the division by zero validations.
    if (expr->value() == 0 || arg_interval.lower() == 0 ||
        arg_interval.upper() == 0)
      throw InvalidArgument("Division by 0.");
  }
}

Interval Div::interval() noexcept {
  auto it = Expression::args().begin();
  Interval first_arg_interval = (*it)->interval();
  double max_value = first_arg_interval.upper();
  double min_value = first_arg_interval.lower();
  for (++it; it != Expression::args().end(); ++it) {
    Interval next_arg_interval = (*it)->interval();
    double div_max = next_arg_interval.upper();
    double div_min = next_arg_interval.lower();
    double max_max = max_value / div_max;
    double max_min = max_value / div_min;
    double min_max = min_value / div_max;
    double min_min = min_value / div_min;
    std::tie(min_value, max_value) =
        std::minmax({max_max, max_min, min_max, min_min});
  }
  assert(min_value <= max_value);
  return Interval::closed(min_value, max_value);
}

}  // namespace mef
}  // namespace scram
