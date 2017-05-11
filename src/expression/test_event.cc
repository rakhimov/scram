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

/// @file test_event.cc
/// Implementations of event occurrence tests.

#include "test_event.h"

#include "src/ext/find_iterator.h"

namespace scram {
namespace mef {

double TestInitiatingEvent::value() noexcept {
  return context_.initiating_event == name_;
}

double TestFunctionalEvent::value() noexcept {
  if (auto it = ext::find(context_.functional_events, name_))
    return it->second == state_;
  return false;
}

}  // namespace mef
}  // namespace scram
