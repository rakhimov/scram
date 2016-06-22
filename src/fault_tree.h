/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file fault_tree.h
/// Fault Tree and Component containers.

#ifndef SCRAM_SRC_FAULT_TREE_H_
#define SCRAM_SRC_FAULT_TREE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "expression.h"

namespace scram {
namespace mef {

/// Component is for logical grouping of events, gates, and other components.
class Component : public Element, public Role {
 public:
  /// Constructs a component assuming
  /// that it exists within some fault tree.
  /// The public or private role of a component is not
  /// for the components itself,
  /// but for the events and parameters of the component.
  /// Component name is not meant to be public;
  /// however, it must be unique within the parent fault tree or component.
  ///
  /// @param[in] name  The name identifier for the component.
  /// @param[in] base_path  The series of containers to get this container.
  /// @param[in] role  The default role for container members.
  ///
  /// @throws LogicError  The name is empty.
  /// @throws InvalidArgument  The name or reference paths are malformed.
  explicit Component(std::string name, std::string base_path = "",
                     RoleSpecifier role = RoleSpecifier::kPublic);

  Component(const Component&) = delete;
  Component& operator=(const Component&) = delete;

  virtual ~Component() {}

  /// @returns The container of component constructs of specific kind
  ///          with construct original names as keys.
  /// @{
  const std::unordered_map<std::string, GatePtr>& gates() const {
    return gates_;
  }
  const std::unordered_map<std::string, BasicEventPtr>& basic_events() const {
    return basic_events_;
  }
  const std::unordered_map<std::string, HouseEventPtr>& house_events() const {
    return house_events_;
  }
  const std::unordered_map<std::string, ParameterPtr>& parameters() const {
    return parameters_;
  }
  const std::unordered_map<std::string, CcfGroupPtr>& ccf_groups() const {
    return ccf_groups_;
  }
  const std::unordered_map<std::string, std::unique_ptr<Component>>&
  components() const {
    return components_;
  }
  /// @}

  /// Adds a gate into this component container.
  ///
  /// @param[in] gate  The gate to be added to this tree.
  ///
  /// @throws ValidationError  The event is already in this container.
  void AddGate(const GatePtr& gate);

  /// Adds a basic event into this component container.
  ///
  /// @param[in] basic_event  The basic event to be added to this tree.
  ///
  /// @throws ValidationError  The event is already in this container.
  void AddBasicEvent(const BasicEventPtr& basic_event);

  /// Adds a house event into this component container.
  ///
  /// @param[in] house_event  The house event to be added to this tree.
  ///
  /// @throws ValidationError  The event is already in this container.
  void AddHouseEvent(const HouseEventPtr& house_event);

  /// Adds a parameter into this component container.
  ///
  /// @param[in] parameter  The parameter to be added to this tree.
  ///
  /// @throws ValidationError  The parameter is already in this container.
  void AddParameter(const ParameterPtr& parameter);

  /// Adds a CCF group and its members into this component container.
  ///
  /// @param[in] ccf_group  The CCF group to be added to this container.
  ///
  /// @throws ValidationError  Duplicate CCF groups
  ///                          or duplicate basic event members.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

  /// Adds a member component container into this component container.
  /// Components are unique.
  /// The ownership is transferred to this component only.
  ///
  /// @param[in] component  The CCF group to be added to this container.
  ///
  /// @throws ValidationError  The component is already in this container.
  void AddComponent(std::unique_ptr<Component> component);

 protected:
  /// Recursively traverses components
  /// to gather gates relevant to the whole component.
  ///
  /// @param[out] gates  Gates belonging to this component
  ///                    and its subcomponents.
  void GatherGates(std::unordered_set<Gate*>* gates);

 private:
  /// Adds an event into this component container.
  ///
  /// @tparam Ptr  The smart pointer type to the event.
  /// @tparam Container  Map with the event's original name as the key.
  ///
  /// @param[in] event  The event to be added to this component.
  /// @param[in,out] container  The destination container.
  ///
  /// @throws ValidationError  The event is already in this container.
  template <class Ptr, class Container>
  void AddEvent(const Ptr& event, Container* container);

  /// Container for component constructs with original names as keys.
  /// @{
  std::unordered_map<std::string, GatePtr> gates_;
  std::unordered_map<std::string, BasicEventPtr> basic_events_;
  std::unordered_map<std::string, HouseEventPtr> house_events_;
  std::unordered_map<std::string, ParameterPtr> parameters_;
  std::unordered_map<std::string, CcfGroupPtr> ccf_groups_;
  std::unordered_map<std::string, std::unique_ptr<Component>> components_;
  /// @}
};

using ComponentPtr = std::unique_ptr<Component>;  ///< Unique system components.

/// Fault tree representation as a container of
/// gates, basic and house events, and other information.
/// Additional functionality of a fault tree includes
/// detection of top events.
class FaultTree : public Component {
 public:
  /// The main constructor of the Fault Tree.
  /// Fault trees are assumed to be public and belong to the root model.
  ///
  /// @param[in] name  The name identifier of this fault tree.
  explicit FaultTree(const std::string& name);

  /// @returns The collected top events of this fault tree.
  const std::vector<const Gate*>& top_events() const { return top_events_; }

  /// Collects top event gates in this fault tree with components.
  /// This function is essential to guess the analysis targets
  /// if the user does not supply any.
  /// If the structure of the fault tree changes,
  /// this function must be called again to update the top events.
  void CollectTopEvents();

 private:
  /// Recursively marks descendant gates as "non-top".
  /// These gates belong to this fault tree only.
  ///
  /// @param[in,out] gate  The ancestor gate.
  /// @param[in] gates  Gates belonging to the whole fault tree with components.
  void MarkNonTopGates(Gate* gate, const std::unordered_set<Gate*>& gates);

  /// Recursively marks descendant gates in formulas as "non-top"
  ///
  /// @param[in] formula  The formula of a gate or another formula.
  /// @param[in] gates  Gates belonging to the whole fault tree with components.
  void MarkNonTopGates(const Formula& formula,
                       const std::unordered_set<Gate*>& gates);

  std::vector<const Gate*> top_events_;  ///< Top events of this fault tree.
};

using FaultTreePtr = std::unique_ptr<FaultTree>;  ///< Unique trees in models.

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_H_
