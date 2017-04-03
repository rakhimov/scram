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

/// @file event_tree.h
/// Event Tree facilities.

#ifndef SCRAM_SRC_EVENT_TREE_H_
#define SCRAM_SRC_EVENT_TREE_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include "element.h"
#include "expression.h"

namespace scram {
namespace mef {

/// Instructions and rules for event tree paths.
class Instruction : private boost::noncopyable {
 public:
  virtual ~Instruction() = 0;
};

/// Instructions are assumed not to be shared.
using InstructionPtr = std::unique_ptr<Instruction>;

/// A collection of instructions.
using InstructionContainer = std::vector<InstructionPtr>;

/// The operation of collecting expressions for event tree sequences.
class CollectExpression : public Instruction {
 public:
  /// @param[in] expression  The expression to multiply
  ///                        the current sequence probability.
  explicit CollectExpression(Expression* expression);

  /// @returns The collected expression for value extraction.
  Expression& expression() const { return *expression_; }

 private:
  Expression* expression_;  ///< The probability expression to multiply.
};

/// Representation of sequences in event trees.
class Sequence : public Element {
 public:
  using Element::Element;

  /// @param[in] instructions  One or more instructions for the sequence.
  ///
  /// @throws LogicError  The instructions are empty.
  void instructions(InstructionContainer instructions);

 private:
  /// Instructions to execute with the sequence.
  InstructionContainer instructions_;
};

/// Sequences are defined in event trees but referenced in other constructs.
using SequencePtr = std::shared_ptr<Sequence>;

/// Representation of functional events in event trees.
class FunctionalEvent : public Element {
 public:
  using Element::Element;
};

/// Functional events are defined in event trees but referenced in the model.
using FunctionalEventPtr = std::shared_ptr<FunctionalEvent>;

/// Event Tree representation with MEF constructs.
class EventTree : public Element, private boost::noncopyable {
 public:
  using Element::Element;

  /// Adds event tree constructs into the container.
  ///
  /// @param[in] element  A unique element defined in this event tree.
  ///
  /// @throws ValidationError  The element is already in this container.
  ///
  /// @{
  void Add(SequencePtr element);
  void Add(FunctionalEventPtr element);
  /// @}

 private:
  /// Containers for unique event tree constructs defined in this event tree.
  /// @{
  ElementTable<SequencePtr> sequences_;
  ElementTable<FunctionalEventPtr> functional_events_;
  /// @}
};

using EventTreePtr = std::unique_ptr<EventTree>;  ///< Unique trees in models.

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EVENT_TREE_H_
