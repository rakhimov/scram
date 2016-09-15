/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file cycle.cc
/// Implementation of helper facilities to work with cycles.

#include "cycle.h"

#include <algorithm>

namespace scram {
namespace mef {
namespace cycle {

std::string PrintCycle(const std::vector<std::string>& cycle) {
  assert(cycle.size() > 1);
  auto it = std::find(cycle.rbegin(), cycle.rend(), cycle.front());
  assert(it != std::prev(cycle.rend()) && "No cycle is provided.");

  std::string result = *it;
  for (++it; it != cycle.rend(); ++it)
    result += "->" + *it;
  return result;
}

}  // namespace cycle
}  // namespace mef
}  // namespace scram
