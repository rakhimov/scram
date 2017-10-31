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

/// @file substitution.cc
/// Implementation of Substitution members.

#include "substitution.h"

#include "error.h"
#include "ext/algorithm.h"

namespace scram {
namespace mef {

void Substitution::Add(BasicEvent* source_event) {
  if (ext::any_of(source_, [source_event](BasicEvent* arg) {
        return arg->id() == source_event->id();
      })) {
    SCRAM_THROW(DuplicateArgumentError("Duplicate source event: " +
                                       source_event->id()));
  }
  source_.push_back(source_event);
}

void Substitution::Validate() const {
  assert(hypothesis_ && "Missing substitution hypothesis.");
  if (ext::any_of(hypothesis_->event_args(), [](const Formula::EventArg& arg) {
        return !boost::get<BasicEvent*>(&arg);
      })) {
    SCRAM_THROW(ValidityError(
        "Substitution hypothesis must be built over basic events only."));
  }
  if (hypothesis_->formula_args().empty() == false) {
    SCRAM_THROW(
        ValidityError("Substitution hypothesis formula cannot be nested."));
  }
}

}  // namespace mef
}  // namespace scram
