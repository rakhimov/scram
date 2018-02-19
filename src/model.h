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
/// Representation for a model container for risk analysis.

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "alignment.h"
#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "event_tree.h"
#include "expression.h"
#include "expression/extern.h"
#include "expression/test_event.h"
#include "fault_tree.h"
#include "instruction.h"
#include "parameter.h"
#include "substitution.h"

namespace scram::mef {

/// This class represents a risk analysis model.
class Model
    : public Element,
      public MultiContainer<Model, InitiatingEvent, EventTree, Sequence, Rule,
                            Alignment, Substitution, FaultTree, BasicEvent,
                            Gate, HouseEvent, Parameter, CcfGroup,
                            ExternLibrary, ExternFunction<void>> {
 public:
  /// Container type identifier string for error messages.
  static constexpr const char* kTypeString = "model";

  /// Only Model is allowed to have an optional name,
  /// while all other Elements require names.
  /// An empty name is an error for Element class invariants as well.
  /// This leads to a nasty magic string based optional name for a model.
  static constexpr const char kDefaultName[] = "__unnamed-model__";

  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  ///
  /// @throws ValidityError  The name is malformed.
  explicit Model(std::string name = "");

  /// @returns true if the model name has not been set.
  bool HasDefaultName() const { return Element::name() == kDefaultName; }

  /// @returns The model name or an empty string for the optional name.
  const std::string& GetOptionalName() const {
    static const std::string empty_name("");
    return HasDefaultName() ? empty_name : Element::name();
  }

  /// Sets the optional name of the model.
  void SetOptionalName(std::string name = "") {
    Element::name(name.empty() ? kDefaultName : std::move(name));
  }

  /// @returns The mission time expression for the whole model.
  /// @{
  const MissionTime& mission_time() const { return *mission_time_; }
  MissionTime& mission_time() { return *mission_time_; }
  /// @}

  /// @returns The context to be used by test-event expressions
  ///          for event-tree walks.
  ///
  /// @note There's only single context for the whole model (i.e., global);
  ///       two event-trees cannot be walked concurrently.
  Context* context() const { return const_cast<Context*>(&context_); }

  /// @returns Table ranges of the elements in the model.
  /// @{
  auto initiating_events() const { return table<InitiatingEvent>(); }
  auto event_trees() const { return table<EventTree>(); }
  auto sequences() const { return table<Sequence>(); }
  auto rules() const { return table<Rule>(); }
  auto fault_trees() const { return table<FaultTree>(); }
  auto alignments() const { return table<Alignment>(); }
  auto substitutions() const { return table<Substitution>(); }
  auto parameters() const { return table<Parameter>(); }
  auto house_events() const { return table<HouseEvent>(); }
  auto basic_events() const { return table<BasicEvent>(); }
  auto gates() const { return table<Gate>(); }
  auto ccf_groups() const { return table<CcfGroup>(); }
  auto libraries() const { return table<ExternLibrary>(); }
  auto extern_functions() const { return table<ExternFunction<void>>(); }
  /// @}

  using Composite::Add;
  using Composite::Remove;

  /// Adds MEF constructs into the model container.
  ///
  /// @param[in] element  An element defined in this model.
  ///
  /// @throws DuplicateElementError  The element is already in the model.
  ///
  /// @{
  void Add(std::unique_ptr<HouseEvent> element) {
    AddEvent(std::move(element));
  }
  void Add(std::unique_ptr<BasicEvent> element) {
    AddEvent(std::move(element));
  }
  void Add(std::unique_ptr<Gate> element) { AddEvent(std::move(element)); }
  void Add(std::unique_ptr<Expression> element) {
    expressions_.emplace_back(std::move(element));
  }
  void Add(std::unique_ptr<Instruction> element) {
    instructions_.emplace_back(std::move(element));
  }
  /// @}

  /// Convenience function to retrieve an event with its ID.
  ///
  /// @param[in] id  The valid ID string of the event.
  ///
  /// @returns The event with its type encoded in variant suitable for formulas.
  ///
  /// @throws UndefinedElement  The event with the given ID is not in the model.
  Formula::ArgEvent GetEvent(std::string_view id);

 private:
  /// @copydoc Model::Add(std::unique_ptr<BasicEvent>).
  template <class T>
  void AddEvent(std::unique_ptr<T> element) {
    CheckDuplicateEvent(*element);
    Composite::Add(std::move(element));
  }

  /// Checks if an event with the same id is already in the model.
  ///
  /// @param[in] event  The event to be tested for duplicate before insertion.
  ///
  /// @throws DuplicateElementError  The element is already in the model.
  void CheckDuplicateEvent(const Event& event);

  /// Constructs w/o id or name stored for the duration of the model lifetime.
  /// @{
  std::vector<std::unique_ptr<Expression>> expressions_;
  std::vector<std::unique_ptr<Instruction>> instructions_;
  /// @}

  std::unique_ptr<MissionTime> mission_time_;  ///< The system mission time.
  Context context_;  ///< The context to be used by test-event expressions.
};

}  // namespace scram::mef
