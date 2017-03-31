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
#include <unordered_set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "parameter.h"

namespace scram {
namespace mef {

/// Component is for logical grouping of events, gates, and other components.
class Component : public Element, public Role, private boost::noncopyable {
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

  virtual ~Component() = default;

  /// @returns The container of component constructs of specific kind
  ///          with construct original names as keys.
  /// @{
  const ElementTable<GatePtr>& gates() const { return gates_; }
  const ElementTable<BasicEventPtr>& basic_events() const {
    return basic_events_;
  }
  const ElementTable<HouseEventPtr>& house_events() const {
    return house_events_;
  }
  const ElementTable<ParameterPtr>& parameters() const { return parameters_; }
  const ElementTable<CcfGroupPtr>& ccf_groups() const { return ccf_groups_; }
  const ElementTable<std::unique_ptr<Component>>& components() const {
    return components_;
  }
  /// @}

  /// Adds MEF constructs into this component container.
  ///
  /// @param[in] element  The element to be added to the container.
  ///
  /// @throws ValidationError  The element is already in this container.
  ///
  /// @{
  void Add(const GatePtr& element);
  void Add(const BasicEventPtr& element);
  void Add(const HouseEventPtr& element);
  void Add(const ParameterPtr& element);
  void Add(const CcfGroupPtr& element);
  void Add(std::unique_ptr<Component> element);
  /// @}

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
  ElementTable<GatePtr> gates_;
  ElementTable<BasicEventPtr> basic_events_;
  ElementTable<HouseEventPtr> house_events_;
  ElementTable<ParameterPtr> parameters_;
  ElementTable<CcfGroupPtr> ccf_groups_;
  ElementTable<std::unique_ptr<Component>> components_;
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
  ///
  /// @pre Gate marks are clear.
  void CollectTopEvents();

 private:
  /// Recursively marks descendant gates
  /// with an unspecified-but-non-clear mark.
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
