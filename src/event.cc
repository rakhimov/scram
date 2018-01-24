/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of Event Class and its derived classes.

#include "event.h"

#include <boost/range/algorithm.hpp>

#include "error.h"
#include "ext/algorithm.h"
#include "ext/variant.h"

namespace scram::mef {

Event::~Event() = default;

HouseEvent HouseEvent::kTrue = []() {
  HouseEvent house_event("__true__");
  house_event.state(true);
  return house_event;
}();
HouseEvent HouseEvent::kFalse("__false__");

void BasicEvent::Validate() const {
  assert(expression_ && "The basic event's expression is not set.");
  EnsureProbability(expression_, Event::name());
}

void Gate::Validate() const {
  assert(formula_ && "The gate formula is missing.");
  // Detect inhibit flavor.
  if (formula_->connective() != kAnd || !Element::HasAttribute("flavor") ||
      Element::GetAttribute("flavor").value != "inhibit") {
    return;
  }
  if (formula_->args().size() != 2) {
    SCRAM_THROW(ValidityError(Element::name() +
                              "INHIBIT gate must have only 2 arguments"));
  }
  int num_conditional =
      boost::count_if(formula_->args(), [](const Formula::Arg& arg) {
        if (arg.complement)
          return false;

        if (BasicEvent* const* basic_event =
                std::get_if<BasicEvent*>(&arg.event)) {
          return (*basic_event)->HasAttribute("flavor") &&
                 (*basic_event)->GetAttribute("flavor").value == "conditional";
        }

        return false;
      });
  if (num_conditional != 1)
    SCRAM_THROW(ValidityError(Element::name() + " : INHIBIT gate must have" +
                              " exactly one conditional event."));
}

Formula::Formula(Connective connective)
    : connective_(connective), min_number_(0) {}

int Formula::min_number() const {
  if (!min_number_)
    SCRAM_THROW(LogicError("Min number is not set."));
  return min_number_;
}

void Formula::min_number(int number) {
  if (connective_ != kAtleast) {
    SCRAM_THROW(
        LogicError("The min number can only be defined for 'atleast' formulas. "
                   "The connective of this formula is '" +
                   std::string(kConnectiveToString[connective_]) + "'."));
  }
  if (number < 2)
    SCRAM_THROW(ValidityError("Min number cannot be less than 2."));
  if (min_number_)
    SCRAM_THROW(LogicError("Trying to re-assign a min number"));

  min_number_ = number;
}

void Formula::Add(ArgEvent event, bool complement) {
  if (complement) {
    if (connective_ == kNull || connective_ == kNot)
      SCRAM_THROW(LogicError("Invalid nesting of a complement arg."));
  }
  if (connective_ == kNot) {
    if (event == ArgEvent(&HouseEvent::kTrue) ||
        event == ArgEvent(&HouseEvent::kFalse)) {
      SCRAM_THROW(LogicError("Invalid nesting of a constant arg."));
    }
  }

  Event* base = ext::as<Event*>(event);
  if (ext::any_of(args_, [&base](const Arg& arg) {
        return ext::as<Event*>(arg.event)->id() == base->id();
      })) {
    SCRAM_THROW(DuplicateArgumentError("Duplicate argument " + base->name()));
  }
  args_.push_back({complement, event});
  if (!base->usage())
    base->usage(true);
}

void Formula::Remove(ArgEvent event) {
  auto it = boost::find_if(
      args_, [&event](const Arg& arg) { return arg.event == event; });
  if (it == args_.end())
    SCRAM_THROW(LogicError("The argument doesn't belong to this formula."));
  args_.erase(it);
}

void Formula::Validate() const {
  switch (connective_) {
    case kAnd:
    case kOr:
    case kNand:
    case kNor:
      if (args_.size() < 2)
        SCRAM_THROW(
            ValidityError("\"" + std::string(kConnectiveToString[connective_]) +
                          "\" connective must have 2 or more arguments."));
      break;
    case kNot:
    case kNull:
      if (args_.size() != 1)
        SCRAM_THROW(
            ValidityError("\"" + std::string(kConnectiveToString[connective_]) +
                          "\" connective must have only one argument."));
      break;
    case kXor:
    case kIff:
    case kImply:
      if (args_.size() != 2)
        SCRAM_THROW(
            ValidityError("\"" + std::string(kConnectiveToString[connective_]) +
                          "\" connective must have exactly 2 arguments."));
      break;
    case kAtleast:
      if (args_.size() <= min_number_)
        SCRAM_THROW(
            ValidityError("\"atleast\" connective must have more arguments "
                          "than its min number " +
                          std::to_string(min_number_) + "."));
  }
}

}  // namespace scram::mef
