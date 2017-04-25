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

}  // namespace mef
}  // namespace scram
