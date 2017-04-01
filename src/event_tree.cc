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

#include "error.h"

namespace scram {
namespace mef {

Instruction::~Instruction() = default;

CollectExpression::CollectExpression(Expression* expression)
    : expression_(expression) {}

void Sequence::instructions(InstructionContainer instructions) {
  if (instructions.empty()) {
    throw LogicError("Sequence " + Element::name() +
                     " requires at least one instruction");
  }
  instructions_ = std::move(instructions);
}

void EventTree::Add(SequencePtr sequence) {
  if (sequences_.count(sequence->name())) {
    throw ValidationError("Duplicate sequence " + sequence->name() +
                          " in event tree " + Element::name());
  }
  sequences_.insert(std::move(sequence));
}

}  // namespace mef
}  // namespace scram
