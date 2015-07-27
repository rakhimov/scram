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
/// @file model.h
/// Representation for a model container for risk analysis.
#ifndef SCRAM_SRC_MODEL_H_
#define SCRAM_SRC_MODEL_H_

#include <string>
#include <utility>

#include <boost/unordered_map.hpp>

#include "element.h"
#include "event.h"

namespace scram {

class CcfGroup;
class Parameter;
class FaultTree;
class Component;

/// @class Model
/// This class represents a model that is defined in one input file.
class Model : public Element {
 public:
  typedef boost::shared_ptr<Parameter> ParameterPtr;
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;
  typedef boost::shared_ptr<FaultTree> FaultTreePtr;

  /// Creates a model container.
  ///
  /// @param[in] name The optional name for the model.
  explicit Model(const std::string& name = "");

  /// @returns The name of the model.
  inline const std::string& name() const { return name_; }

  /// @returns Defined fault trees in the model.
  inline const boost::unordered_map<std::string, FaultTreePtr>&
      fault_trees() const {
    return fault_trees_;
  }

  /// @returns Parameters defined for this model.
  inline const boost::unordered_map<std::string, ParameterPtr>&
      parameters() const {
    return parameters_;
  }

  /// @returns House events defined for this model.
  inline const boost::unordered_map<std::string, HouseEventPtr>&
      house_events() const {
    return house_events_;
  }

  /// @returns Basic events defined for this model.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      basic_events() const {
    return basic_events_;
  }

  /// @returns Gates defined for this model.
  inline const boost::unordered_map<std::string, GatePtr>& gates() const {
    return gates_;
  }

  /// @returns CCF groups defined for this model.
  inline const boost::unordered_map<std::string, CcfGroupPtr>&
      ccf_groups() const {
    return ccf_groups_;
  }

  /// Adds a fault tree into the model container.
  ///
  /// @param[in] fault_tree A fault tree defined in this model.
  ///
  /// @throws RedefinitionError The model has a container with the same name.
  void AddFaultTree(const FaultTreePtr& fault_tree);

  /// Adds a parameter that is used in this model's expressions.
  ///
  /// @param[in] parameter A parameter defined in this model.
  ///
  /// @throws RedefinitionError The model has a parameter with the same name.
  void AddParameter(const ParameterPtr& parameter);

  /// Finds a parameter from a reference. The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference Reference string to the parameter.
  /// @param[in] base_path The series of containers indicating the scope.
  ///
  /// @returns Pointer to the parameter found by following the given reference.
  ///
  /// @throws ValidationError There are problems with referencing.
  ParameterPtr GetParameter(const std::string& reference,
                            const std::string& base_path);

  /// Finds an event from a reference. The reference is not case sensitive and
  /// can contain the identifier, full path, or local path. The returned event
  /// may be a basic event, house event, or gate. This information is
  /// communicated with the return value.
  ///
  /// @param[in] reference Reference string to the event.
  /// @param[in] base_path The series of containers indicating the scope.
  ///
  /// @returns A pair of the pointer to the event and its type("gate",
  ///          "basic-event", "house-event").
  ///
  /// @throws ValidationError There are problems with referencing.
  /// @throws LogicError The given base path is invalid.
  std::pair<EventPtr, std::string> GetEvent(const std::string& reference,
                                            const std::string& base_path);

  /// Adds a house event that is used in this model.
  ///
  /// @param[in] house_event A house event defined in this model.
  ///
  /// @throws RedefinitionError An event with the same name already exists.
  void AddHouseEvent(const HouseEventPtr& house_event);

  /// Finds a house event from a reference. The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference Reference string to the house event.
  /// @param[in] base_path The series of containers indicating the scope.
  ///
  /// @returns Pointer to the house event found by following the reference.
  ///
  /// @throws ValidationError There are problems with referencing.
  HouseEventPtr GetHouseEvent(const std::string& reference,
                              const std::string& base_path);

  /// Adds a basic event that is used in this model.
  ///
  /// @param[in] basic_event A basic event defined in this model.
  ///
  /// @throws RedefinitionError An event with the same name already exists.
  void AddBasicEvent(const BasicEventPtr& basic_event);

  /// Finds a basic event from a reference. The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference Reference string to the basic event.
  /// @param[in] base_path The series of containers indicating the scope.
  ///
  /// @returns Pointer to the basic event found by following the reference.
  ///
  /// @throws ValidationError There are problems with referencing.
  BasicEventPtr GetBasicEvent(const std::string& reference,
                              const std::string& base_path);

  /// Adds a gate that is used in this model's fault trees or components.
  ///
  /// @param[in] gate A gate defined in this model.
  ///
  /// @throws RedefinitionError An event with the same name already exists.
  void AddGate(const GatePtr& gate);

  /// Finds a gate from a reference. The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference Reference string to the gate.
  /// @param[in] base_path The series of containers indicating the scope.
  ///
  /// @returns Pointer to the gate found by following the reference.
  ///
  /// @throws ValidationError There are problems with referencing.
  GatePtr GetGate(const std::string& reference, const std::string& base_path);

  /// Adds a CCF group that is used in this model's fault trees.
  ///
  /// @param[in] ccf_group A CCF group defined in this model.
  ///
  /// @throws RedefinitionError The model has a CCF group with the same name.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

 private:
  typedef boost::shared_ptr<Component> ComponentPtr;

  /// Helper function to find the scope container for references.
  ///
  /// @param[in] base_path The series of containers to get the container.
  ///
  /// @returns A fault tree or component from the base path if any.
  ///
  /// @throws LogicError There's missing container in the path.
  ComponentPtr GetContainer(const std::string& base_path);

  /// Helper function to find the local container for references.
  ///
  /// @param[in] reference The reference to the target element.
  /// @param[in] scope The fault tree or component as a scope.
  ///
  /// @returns A fault tree or component from the reference if any.
  ComponentPtr GetLocalContainer(const std::string& reference,
                                 const ComponentPtr& scope);

  /// Helper function to find the global container for references.
  ///
  /// @param[in] reference The reference to the target element.
  ///
  /// @returns A fault tree or component from the reference.
  ///
  /// @throws ValidationError There's missing container in the path.
  ComponentPtr GetGlobalContainer(const std::string& reference);

  std::string name_;  ///< The name of the model.

  /// A collection of fault trees.
  boost::unordered_map<std::string, FaultTreePtr> fault_trees_;

  /// Container for fully defined gates.
  boost::unordered_map<std::string, GatePtr> gates_;

  /// Container for fully defined house events.
  boost::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for fully defined basic events.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for defined parameters or variables.
  boost::unordered_map<std::string, ParameterPtr> parameters_;

  /// A collection of common cause failure groups.
  boost::unordered_map<std::string, CcfGroupPtr> ccf_groups_;
};

}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
