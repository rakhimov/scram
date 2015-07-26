/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

namespace scram {
namespace cycle {

std::string PrintCycle(const std::vector<std::string>& cycle) {
  assert(cycle.size() > 1);
  std::vector<std::string>::const_iterator it = cycle.begin();
  std::string cycle_start = *it;
  std::string result = "->" + cycle_start;
  for (++it; *it != cycle_start; ++it) {
    assert(it != cycle.end());
    result = "->" + *it + result;
  }
  result = cycle_start + result;
  return result;
}

}  // namespace cycle
}  // namespace scram
