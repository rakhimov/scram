/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

/// @file conditional.cc
/// Implementation of conditional expressions.

#include "conditional.h"

#include <algorithm>

namespace scram {
namespace mef {

Interval Ite::interval() noexcept {
  assert(args().size() == 3);
  Interval then_interval = args()[1]->interval();
  Interval else_interval = args()[2]->interval();
  return Interval::closed(
      std::min(then_interval.lower(), else_interval.lower()),
      std::max(then_interval.upper(), else_interval.upper()));
}

Switch::Switch(std::vector<Case> cases, Expression* default_value)
    : ExpressionFormula({default_value}),
      cases_(std::move(cases)),
      default_value_(*default_value) {
  for (auto& case_arm : cases_) {
    Expression::AddArg(&case_arm.condition);
    Expression::AddArg(&case_arm.value);
  }
}

Interval Switch::interval() noexcept {
  Interval default_interval = default_value_.interval();
  double min_value = default_interval.lower();
  double max_value = default_interval.upper();
  for (auto& case_arm : cases_) {
    Interval case_interval = case_arm.value.interval();
    min_value = std::min(min_value, case_interval.lower());
    max_value = std::max(max_value, case_interval.upper());
  }
  return Interval::closed(min_value, max_value);
}

}  // namespace mef
}  // namespace scram
