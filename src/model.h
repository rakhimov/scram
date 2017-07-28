/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file model.h
/// Representation for a model container for risk analysis.

#ifndef SCRAM_SRC_MODEL_H_
#define SCRAM_SRC_MODEL_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "event_tree.h"
#include "expression.h"
#include "expression/test_event.h"
#include "fault_tree.h"
#include "parameter.h"

namespace scram {
namespace mef {

/// This class represents a risk analysis model.
class Model : public Element, private boost::noncopyable {
 public:
  /// Only Model is allowed to have an optional name,
  /// while all other Elements require names.
  /// An empty name is an error for Element class invariants as well.
  /// This leads to a nasty magic string based optional name for a model.
  static const char kDefaultName[];

  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  ///
  /// @throws InvalidArgument  The name is malformed.
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

  /// @returns The context to be used by test-event expressions
  ///          for event-tree walks.
  ///
  /// @note There's only single context for the whole model (i.e., global);
  ///       two event-trees cannot be walked concurrently.
  Context* context() const { return const_cast<Context*>(&context_); }

  /// @returns Defined constructs in the model.
  /// @{
  const ElementTable<InitiatingEventPtr>& initiating_events() const {
    return initiating_events_;
  }
  const ElementTable<EventTreePtr>& event_trees() const { return event_trees_; }
  const ElementTable<SequencePtr>& sequences() const { return sequences_; }
  const ElementTable<RulePtr>& rules() const { return rules_; }
  const ElementTable<FaultTreePtr>& fault_trees() const { return fault_trees_; }
  const IdTable<ParameterPtr>& parameters() const { return parameters_; }
  const MissionTime& mission_time() const { return *mission_time_; }
  MissionTime& mission_time() { return *mission_time_; }
  const IdTable<HouseEventPtr>& house_events() const { return house_events_; }
  const IdTable<BasicEventPtr>& basic_events() const { return basic_events_; }
  const IdTable<GatePtr>& gates() const { return gates_; }
  const IdTable<CcfGroupPtr>& ccf_groups() const { return ccf_groups_; }
  /// @}

  /// Adds MEF constructs into the model container.
  ///
  /// @param[in] element  An element defined in this model.
  ///
  /// @throws RedefinitionError  The element is already defined in the model.
  ///
  /// @{
  void Add(InitiatingEventPtr element);
  void Add(EventTreePtr element);
  void Add(SequencePtr element);
  void Add(RulePtr element);
  void Add(FaultTreePtr element);
  void Add(ParameterPtr element);
  void Add(HouseEventPtr element);
  void Add(BasicEventPtr element);
  void Add(GatePtr element);
  void Add(CcfGroupPtr element);
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
  Formula::EventArg GetEvent(const std::string& id);

  /// Removes MEF constructs from the model container.
  ///
  /// @param[in] element  An element defined in this model.
  ///
  /// @returns The removed element.
  ///
  /// @throws UndefinedElement  The element cannot be found.
  /// @{
  HouseEventPtr Remove(HouseEvent* element);
  BasicEventPtr Remove(BasicEvent* element);
  GatePtr Remove(Gate* element);
  FaultTreePtr Remove(FaultTree* element);
  /// @}

 private:
  /// Checks if an event with the same id is already in the model.
  ///
  /// @param[in] event  The event to be tested for duplicate before insertion.
  ///
  /// @throws RedefinitionError  The element is already defined in the model.
  void CheckDuplicateEvent(const Event& event);

  /// A collection of defined constructs in the model.
  /// @{
  ElementTable<InitiatingEventPtr> initiating_events_;
  ElementTable<EventTreePtr> event_trees_;
  ElementTable<SequencePtr> sequences_;
  ElementTable<RulePtr> rules_;
  ElementTable<FaultTreePtr> fault_trees_;
  IdTable<GatePtr> gates_;
  IdTable<HouseEventPtr> house_events_;
  IdTable<BasicEventPtr> basic_events_;
  IdTable<ParameterPtr> parameters_;
  std::unique_ptr<MissionTime> mission_time_;
  IdTable<CcfGroupPtr> ccf_groups_;
  std::vector<std::unique_ptr<Expression>> expressions_;
  std::vector<std::unique_ptr<Instruction>> instructions_;
  /// @}
  Context context_;  ///< The context to be used by test-event expressions.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
