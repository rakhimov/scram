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
/// Event Tree facilities.

#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "element.h"
#include "ext/variant.h"
#include "instruction.h"

namespace scram::mef {

/// Representation of sequences in event trees.
class Sequence : public Element, public Usage {
 public:
  /// Type string for error messages.
  static constexpr const char* kTypeString = "sequence";

  using Element::Element;

  /// @param[in] instructions  Zero or more instructions for the sequence.
  void instructions(std::vector<Instruction*> instructions) {
    instructions_ = std::move(instructions);
  }

  /// @returns The instructions to be applied at this sequence.
  const std::vector<Instruction*>& instructions() const {
    return instructions_;
  }

 private:
  /// Instructions to execute with the sequence.
  std::vector<Instruction*> instructions_;
};

class EventTree;  // Manages the order assignment to functional events.

/// Representation of functional events in event trees.
class FunctionalEvent : public Element, public Usage {
 public:
  /// Type string for error messages.
  static constexpr const char* kTypeString = "functional event";

  using Element::Element;

  /// @returns The order of the functional event in the event tree.
  /// @returns 0 if no order has been assigned.
  int order() const { return order_; }

  /// Sets the functional event order.
  void order(int order) { order_ = order; }

 private:
  int order_ = 0;  ///< The order of the functional event.
};

class Fork;
class NamedBranch;

/// The branch representation in event trees.
class Branch {
 public:
  /// The types of possible branch end-points.
  using Target = std::variant<Sequence*, Fork*, NamedBranch*>;

  /// Sets the instructions to execute at the branch.
  void instructions(std::vector<Instruction*> instructions) {
    instructions_ = std::move(instructions);
  }

  /// @returns The instructions to execute at the branch.
  const std::vector<Instruction*>& instructions() const {
    return instructions_;
  }

  /// Sets the target for the branch.
  void target(Target target) { target_ = std::move(target); }

  /// @returns The target semantics or end-points of the branch.
  ///
  /// @pre The target has been set.
  const Target& target() const {
    assert(ext::as<bool>(target_));
    return target_;
  }

 private:
  std::vector<Instruction*> instructions_;  ///< Zero or more instructions.
  Target target_;  ///< The target semantics of the branch.
};

/// Named branches that can be referenced and reused.
class NamedBranch : public Element,
                    public Branch,
                    public NodeMark,
                    public Usage {
 public:
  static constexpr const char* kTypeString = "branch";  ///< For error message.

  using Element::Element;
};

/// Functional-event state paths in event trees.
class Path : public Branch {
 public:
  /// @param[in] state  State identifier string for functional event.
  ///
  /// @throws LogicError  The string is empty or malformed.
  explicit Path(std::string state);

  /// @returns The state of a functional event.
  const std::string& state() const { return state_; }

 private:
  std::string state_;  ///< The state identifier.
};

/// Functional event forks.
class Fork {
 public:
  /// @param[in] functional_event  The source functional event.
  /// @param[in] paths  The fork paths with functional event states.
  ///
  /// @throws ValidityError  The path states are duplicated.
  Fork(const FunctionalEvent& functional_event, std::vector<Path> paths);

  /// @returns The functional event of the fork.
  const FunctionalEvent& functional_event() const { return functional_event_; }

  /// @returns The fork paths with functional event states.
  /// @{
  const std::vector<Path>& paths() const { return paths_; }
  std::vector<Path>& paths() { return paths_; }
  /// @}

 private:
  const FunctionalEvent& functional_event_;  ///< The fork source.
  std::vector<Path> paths_;  ///< The non-empty collection of fork paths.
};

/// Event Tree representation with MEF constructs.
class EventTree : public Element,
                  public Usage,
                  public Composite<Container<EventTree, Sequence, false>,
                                   Container<EventTree, FunctionalEvent>,
                                   Container<EventTree, NamedBranch>> {
 public:
  /// Container and element type string for error messages.
  static constexpr const char* kTypeString = "event tree";

  using Element::Element;

  /// @returns The initial state branch of the event tree.
  const Branch& initial_state() const { return initial_state_; }

  /// Sets the initial state of the event tree.
  void initial_state(Branch branch) { initial_state_ = std::move(branch); }

  /// @returns The table range of event tree elements of specific kind
  ///          with element original names as keys.
  /// @{
  auto sequances() const { return table<Sequence>(); }
  auto functional_events() const { return table<FunctionalEvent>(); }
  auto branches() const { return table<NamedBranch>(); }
  /// @}

  using Composite::Add;
  using Composite::Remove;

  /// Registers an event tree fork.
  ///
  /// @param[in] fork  The fork in this event tree.
  void Add(std::unique_ptr<Fork> fork) { forks_.push_back(std::move(fork)); }

 private:
  Branch initial_state_;  ///< The starting point.
  std::vector<std::unique_ptr<Fork>> forks_;  ///< Lifetime management of forks.
};

/// Event-tree Initiating Event.
class InitiatingEvent : public Element, public Usage {
 public:
  /// Type string for error messages.
  static constexpr const char* kTypeString = "initiating event";

  using Element::Element;

  /// Associates an event tree to the initiating event.
  ///
  /// @param[in] event_tree  Fully initialized event tree container.
  void event_tree(EventTree* event_tree) {
    assert(!event_tree_ && event_tree && "Resetting or un-setting event tree.");
    event_tree_ = event_tree;
  }

  /// @returns The event tree of the initiating event.
  ///          nullptr if the event tree is not set.
  /// @{
  EventTree* event_tree() const { return event_tree_; }
  EventTree* event_tree() { return event_tree_; }
  /// @}

 private:
  EventTree* event_tree_ = nullptr;  ///< The optional event tree specification.
};

}  // namespace scram::mef
