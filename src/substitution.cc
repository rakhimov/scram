/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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
/// Implementation of Substitution members.

#include "substitution.h"

#include "error.h"
#include "ext/algorithm.h"

namespace scram::mef {

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
  if (ext::any_of(hypothesis_->args(), [](const Formula::Arg& arg) {
        return !std::holds_alternative<BasicEvent*>(arg.event);
      })) {
    SCRAM_THROW(ValidityError(
        "Substitution hypothesis must be built over basic events only."));
  }

  if (ext::any_of(hypothesis_->args(),
                  [](const Formula::Arg& arg) { return arg.complement; })) {
    SCRAM_THROW(ValidityError("Substitution hypotheses must be coherent."));
  }

  if (declarative()) {
    switch (hypothesis_->connective()) {
      case kNull:
      case kAnd:
      case kAtleast:
      case kOr:
        break;
      default:
        SCRAM_THROW(ValidityError("Substitution hypotheses must be coherent."));
    }
    const bool* constant = std::get_if<bool>(&target_);
    if (constant && *constant)
      SCRAM_THROW(ValidityError("Substitution has no effect."));
  } else {  // Non-declarative.
    switch (hypothesis_->connective()) {
      case kNull:
      case kAnd:
      case kOr:
        break;
      default:
        SCRAM_THROW(
            ValidityError("Non-declarative substitution hypotheses only allow "
                          "AND/OR/NULL connectives."));
    }
    const bool* constant = std::get_if<bool>(&target_);
    if (constant && !*constant)
      SCRAM_THROW(ValidityError("Substitution source set is irrelevant."));
  }
}

std::optional<Substitution::Type> Substitution::type() const {
  auto in_hypothesis = [this](const BasicEvent* source_arg) {
    return ext::any_of(hypothesis_->args(),
                       [source_arg](const Formula::Arg& arg) {
                         return std::get<BasicEvent*>(arg.event) == source_arg;
                       });
  };

  auto is_mutually_exclusive = [](const Formula& formula) {
    switch (formula.connective()) {
      case kAtleast:
        return formula.min_number() == 2;
      case kAnd:
        return formula.args().size() == 2;
      default:
        return false;
    }
  };

  if (source_.empty()) {
    if (const bool* constant = std::get_if<bool>(&target_)) {
      assert(!*constant && "Substitution has no effect.");
      if (is_mutually_exclusive(*hypothesis_))
        return kDeleteTerms;
    } else if (std::holds_alternative<BasicEvent*>(target_)) {
      if (hypothesis_->connective() == kAnd)
        return kRecoveryRule;
    }
    return {};
  }
  if (!std::holds_alternative<BasicEvent*>(target_))
    return {};
  if (hypothesis_->connective() != kAnd && hypothesis_->connective() != kNull)
    return {};

  if (source_.size() == hypothesis_->args().size()) {
    if (ext::all_of(source_, in_hypothesis))
      return kRecoveryRule;
  } else if (source_.size() == 1) {
    if (in_hypothesis(source_.front()))
      return kExchangeEvent;
  }
  return {};
}

}  // namespace scram::mef
