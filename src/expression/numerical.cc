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
  auto* arg_one = args.front();
  double arg_value = arg_one->value();
  if (arg_value > 1 || arg_value < -1)
    throw InvalidArgument("Arc cos argument value must be in [-1, 1].");
  if (!boost::icl::within(arg_one->interval(), Interval::closed(-1, 1)))
    throw InvalidArgument("Arc cos argument sample domain must be in [-1, 1].");
}

}  // namespace mef
}  // namespace scram
