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

#include <limits>

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
  try {
    EnsureProbability(expression_);
  } catch (DomainError& err) {
    err << errinfo_element(Event::name(), kTypeString);
    throw;
  }
}

void Formula::ArgSet::Add(ArgEvent event, bool complement) {
  Event* base = ext::as<Event*>(event);
  if (ext::any_of(args_, [&base](const Arg& arg) {
        return ext::as<Event*>(arg.event)->id() == base->id();
      })) {
    SCRAM_THROW(DuplicateElementError())
        << errinfo_element(base->id(), "event");
  }
  args_.push_back({complement, event});
  if (!base->usage())
    base->usage(true);
}

void Formula::ArgSet::Remove(ArgEvent event) {
  auto it = boost::find_if(
      args_, [&event](const Arg& arg) { return arg.event == event; });
  if (it == args_.end())
    SCRAM_THROW(LogicError("The event is not in the argument set."));
  args_.erase(it);
}

Formula::Formula(Connective connective, ArgSet args,
                 std::optional<int> min_number, std::optional<int> max_number)
    : connective_(connective),
      min_number_(min_number.value_or(0)),
      max_number_(max_number.value_or(0)),
      args_(std::move(args)) {
  ValidateMinMaxNumber(min_number, max_number);

  try {
    ValidateConnective(min_number, max_number);
  } catch (ValidityError& err) {
    err << errinfo_connective(kConnectiveToString[connective_]);
    throw;
  }

  for (const Arg& arg : args_.data())
    ValidateNesting(arg);
}

std::optional<int> Formula::min_number() const {
  if (connective_ == kAtleast || connective_ == kCardinality)
    return min_number_;
  return {};
}

std::optional<int> Formula::max_number() const {
  if (connective_ == kCardinality)
    return max_number_;
  return {};
}

void Formula::Swap(ArgEvent current, ArgEvent other) {
  auto it = boost::find_if(args_.data(), [&current](const Arg& arg) {
    return arg.event == current;
  });
  if (it == args_.data().end())
    SCRAM_THROW(LogicError("The current event is not in the formula."));

  Event* base = ext::as<Event*>(other);
  if (ext::any_of(args_.data(), [&current, &base](const Arg& arg) {
        return arg.event != current &&
               ext::as<Event*>(arg.event)->id() == base->id();
      })) {
    SCRAM_THROW(DuplicateElementError())
        << errinfo_element(base->id(), "event");
  }

  ValidateNesting({it->complement, other});

  if (!base->usage())
    base->usage(true);

  it->event.swap(other);
}

void Formula::ValidateMinMaxNumber(std::optional<int> min_number,
                                   std::optional<int> max_number) {
  assert(!min_number ||
         std::numeric_limits<decltype(min_number_)>::max() >= *min_number);
  assert(!max_number ||
         std::numeric_limits<decltype(max_number_)>::max() >= *max_number);

  if (min_number) {
    if (*min_number < 0)
      SCRAM_THROW(LogicError("The min number cannot be negative."))
          << errinfo_value(std::to_string(*min_number));

    if (connective_ != kAtleast && connective_ != kCardinality) {
      SCRAM_THROW(
          LogicError("The min number can only be defined for 'atleast' "
                     "or 'cardinality' connective."))
          << errinfo_connective(kConnectiveToString[connective_]);
    }
  }

  if (max_number) {
    if (*max_number < 0)
      SCRAM_THROW(LogicError("The max number cannot be negative."))
          << errinfo_value(std::to_string(*max_number));

    if (connective_ != kCardinality) {
      SCRAM_THROW(LogicError(
          "The max number can only be defined for 'cardinality' connective."))
          << errinfo_connective(kConnectiveToString[connective_]);
    }

    if (min_number && *min_number > *max_number)
      SCRAM_THROW(ValidityError(
          "The connective min number cannot be greater than max number."))
          << errinfo_value(std::to_string(*min_number) + " > " +
                           std::to_string(*max_number));
  }
}

void Formula::ValidateConnective(std::optional<int> min_number,
                                 std::optional<int> max_number) {
  switch (connective_) {
    case kAnd:
    case kOr:
    case kNand:
    case kNor:
      if (args_.size() < 2)
        SCRAM_THROW(
            ValidityError("The connective must have 2 or more arguments."));
      break;
    case kNot:
    case kNull:
      if (args_.size() != 1)
        SCRAM_THROW(
            ValidityError("The connective must have only one argument."));
      break;
    case kXor:
    case kIff:
    case kImply:
      if (args_.size() != 2)
        SCRAM_THROW(
            ValidityError("The connective must have exactly 2 arguments."));
      break;
    case kAtleast:
      if (!min_number)
        SCRAM_THROW(
            ValidityError("The connective requires min number for its args."));

      if (min_number_ < 2)
        SCRAM_THROW(ValidityError("Min number cannot be less than 2."))
            << errinfo_value(std::to_string(min_number_));

      if (args_.size() <= min_number_) {
        SCRAM_THROW(
            ValidityError("The connective must have more arguments "
                          "than its min number."))
            << errinfo_value(std::to_string(args_.size()) +
                             " <= " + std::to_string(min_number_));
      }
      break;
    case kCardinality:
      if (!min_number || !max_number)
        SCRAM_THROW(ValidityError(
            "The connective requires min and max numbers for args."));
      if (args_.empty())
        SCRAM_THROW(
            ValidityError("The connective requires one or more arguments."));

      if (args_.size() < max_number_)
        SCRAM_THROW(
            ValidityError("The connective max number cannot be greater than "
                          "the number of arguments."))
            << errinfo_value(std::to_string(max_number_) + " > " +
                             std::to_string(args_.size()));
  }
}

void Formula::ValidateNesting(const Arg& arg) {
  if (arg.complement) {
    if (connective_ == kNull || connective_ == kNot)
      SCRAM_THROW(LogicError("Invalid nesting of a complement arg."));
  }
  if (connective_ == kNot) {
    if (arg.event == ArgEvent(&HouseEvent::kTrue) ||
        arg.event == ArgEvent(&HouseEvent::kFalse)) {
      SCRAM_THROW(LogicError("Invalid nesting of a constant arg."));
    }
  }
}

}  // namespace scram::mef
