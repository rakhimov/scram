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

/// @file event_tree.cc
/// Implementation of event tree facilities.

#include "event_tree.h"

#include <algorithm>

#include "error.h"

namespace scram {
namespace mef {

Path::Path(std::string state) : state_(std::move(state)) {
  if (state_.empty())
    throw LogicError("The state string for functional events cannot be empty");
}

Fork::Fork(const FunctionalEvent& functional_event, std::vector<Path> paths)
    : functional_event_(functional_event), paths_(std::move(paths)) {
  // There are expected to be very few paths (2 in most cases),
  // so quadratic check is not a problem.
  for (auto it = paths_.begin(); it != paths_.end(); ++it) {
    auto it_find =
        std::find_if(std::next(it), paths_.end(), [&it](const Path& fork_path) {
          return fork_path.state() == it->state();
        });
    if (it_find != paths_.end())
      throw ValidationError("Duplicate state '" + it->state() +
                            "' path in fork " + functional_event_.name());
  }
}

void EventTree::Add(SequencePtr sequence) {
  mef::AddElement<ValidationError>(std::move(sequence), &sequences_,
                                   "Duplicate sequence: ");
}

void EventTree::Add(FunctionalEventPtr functional_event) {
  assert(functional_event->order() == 0 && "Non-unique functional event.");
  auto& unordered_event = *functional_event;
  mef::AddElement<ValidationError>(std::move(functional_event),
                                   &functional_events_,
                                   "Duplicate functional event: ");
  unordered_event.order(functional_events_.size());
}

void EventTree::Add(NamedBranchPtr branch) {
  mef::AddElement<ValidationError>(std::move(branch), &branches_,
                                   "Duplicate named branch: ");
}

}  // namespace mef
}  // namespace scram
