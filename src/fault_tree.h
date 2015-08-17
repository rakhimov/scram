/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "element.h"
#include "event.h"

namespace scram {

class CcfGroup;
class Parameter;

/// @class Component
/// Component is for logical grouping of events, gates, and other components.
class Component : public Element, public Role {
 public:
  typedef std::shared_ptr<Gate> GatePtr;
  typedef std::shared_ptr<BasicEvent> BasicEventPtr;
  typedef std::shared_ptr<HouseEvent> HouseEventPtr;
  typedef std::shared_ptr<Parameter> ParameterPtr;
  typedef std::shared_ptr<CcfGroup> CcfGroupPtr;
  typedef std::shared_ptr<Component> ComponentPtr;

  /// Constructs a component assuming
  /// that it exists within some fault tree.
  /// The public or private role of a component is not
  /// for the components itself,
  /// but for the events and parameters of the component.
  /// Component name is not meant to be public;
  /// however, it must be unique within the parent fault tree or component.
  ///
  /// @param[in] name The name identificator for the component.
  /// @param[in] base_path The series of containers to get this container.
  /// @param[in] is_public A flag to define public or private role for members.
  explicit Component(const std::string& name, const std::string& base_path = "",
                     bool is_public = true);

  virtual ~Component() {}

  /// @returns The name of this component.
  inline const std::string& name() const { return name_; }

  /// @returns The container of all gates of this component
  ///          with lower-case names as keys.
  inline const std::unordered_map<std::string, GatePtr>& gates() const {
    return gates_;
  }

  /// @returns The container of all basic events of this component
  ///          with lower-case names as keys.
  inline const std::unordered_map<std::string, BasicEventPtr>&
      basic_events() const {
    return basic_events_;
  }

  /// @returns The container of house events of this component
  ///          with lower-case names as keys.
  inline const std::unordered_map<std::string, HouseEventPtr>&
      house_events() const {
    return house_events_;
  }

  /// @returns The container of parameters of this component
  ///          with lower-case names as keys.
  inline const std::unordered_map<std::string, ParameterPtr>&
      parameters() const {
    return parameters_;
  }

  /// @returns CCF groups belonging to this component
  ///          with lower-case names as keys.
  inline const std::unordered_map<std::string, CcfGroupPtr>&
      ccf_groups() const {
    return ccf_groups_;
  }

  /// @returns Components in this component container
  ///          with lower-case names as keys.
  inline const std::unordered_map<std::string, ComponentPtr>&
      components() const {
    return components_;
  }

  /// Adds a gate into this component container.
  ///
  /// @param[in] gate The gate to be added to this tree.
  ///
  /// @throws ValidationError The event is already in this container.
  void AddGate(const GatePtr& gate);

  /// Adds a basic event into this component container.
  ///
  /// @param[in] basic_event The basic event to be added to this tree.
  ///
  /// @throws ValidationError The event is already in this container.
  void AddBasicEvent(const BasicEventPtr& basic_event);

  /// Adds a house event into this component container.
  ///
  /// @param[in] house_event The house event to be added to this tree.
  ///
  /// @throws ValidationError The event is already in this container.
  void AddHouseEvent(const HouseEventPtr& house_event);

  /// Adds a parameter into this component container.
  ///
  /// @param[in] parameter The parameter to be added to this tree.
  ///
  /// @throws ValidationError The parameter is already in this container.
  void AddParameter(const ParameterPtr& parameter);

  /// Adds a CCF group and its members into this component container.
  ///
  /// @param[in] ccf_group The CCF group to be added to this container.
  ///
  /// @throws ValidationError Duplicate CCF groups
  ///                         or duplicate basic event members.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

  /// Adds a component container into this component container.
  ///
  /// @param[in] component The CCF group to be added to this container.
  ///
  /// @throws ValidationError The component is already in this container.
  void AddComponent(const ComponentPtr& component);

 protected:
  /// Recursively traverses components
  /// to gather gates relevant to the whole component.
  ///
  /// @param[out] gates Gates belonging to this component
  ///                   and its subcomponents.
  void GatherGates(std::unordered_set<GatePtr>* gates);

 private:
  std::string name_;  ///< The name of this component.

  /// Container for gates with lower-case names as keys.
  std::unordered_map<std::string, GatePtr> gates_;

  /// Container for basic events with lower-case names as keys.
  std::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for house events with lower-case names as keys.
  std::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for parameters with lower-case names as keys.
  std::unordered_map<std::string, ParameterPtr> parameters_;

  /// Container for CCF groups with lower-case names as keys.
  std::unordered_map<std::string, CcfGroupPtr> ccf_groups_;

  /// Container for components with lower-case names as keys.
  std::unordered_map<std::string, ComponentPtr> components_;
};

/// @class FaultTree
/// Fault tree representation as a container of
/// gates, basic and house events, and other information.
/// Additional functionality of a fault tree includes
/// detection of top events.
class FaultTree : public Component {
 public:
  typedef std::shared_ptr<Gate> GatePtr;

  /// The main constructor of the Fault Tree.
  /// Fault trees are assumed to be public and belong to the root model.
  ///
  /// @param[in] name The name identificator of this fault tree.
  explicit FaultTree(const std::string& name);

  /// @returns The collected top events of this fault tree.
  inline const std::vector<GatePtr>& top_events() const { return top_events_; }

  /// Collects top event gates in this fault tree with components.
  /// This function is essential to guess the analysis targets
  /// if the user does not supply any.
  /// If the structure of the fault tree changes,
  /// this function must be called again to update the top events.
  void CollectTopEvents();

 private:
  typedef std::shared_ptr<Formula> FormulaPtr;

  /// Recursively marks descendant gates as "non-top".
  /// These gates belong to this fault tree only.
  ///
  /// @param[in] gate The ancestor gate.
  /// @param[in] gates Gates belonging to the whole fault tree with components.
  void MarkNonTopGates(const GatePtr& gate,
                       const std::unordered_set<GatePtr>& gates);

  /// Recursively marks descendant gates in formulas as "non-top"
  ///
  /// @param[in] formula The formula of a gate or another formula.
  /// @param[in] gates Gates belonging to the whole fault tree with components.
  void MarkNonTopGates(const FormulaPtr& formula,
                       const std::unordered_set<GatePtr>& gates);

  std::vector<GatePtr> top_events_;  ///< Top events of this fault tree.
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_H_
