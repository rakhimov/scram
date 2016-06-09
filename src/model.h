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

/// @file model.h
/// Representation for a model container for risk analysis.

#ifndef SCRAM_SRC_MODEL_H_
#define SCRAM_SRC_MODEL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "expression.h"
#include "fault_tree.h"

namespace scram {
namespace mef {

/// This class represents a risk analysis model.
class Model : public Element {
 public:
  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  explicit Model(std::string name = "");

  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

  /// @returns Defined constructs in the model.
  /// @{
  const std::unordered_map<std::string, FaultTreePtr>& fault_trees() const {
    return fault_trees_;
  }
  const std::unordered_map<std::string, ParameterPtr>& parameters() const {
    return parameters_;
  }
  const std::unordered_map<std::string, HouseEventPtr>& house_events() const {
    return house_events_;
  }
  const std::unordered_map<std::string, BasicEventPtr>& basic_events() const {
    return basic_events_;
  }
  const std::unordered_map<std::string, GatePtr>& gates() const {
    return gates_;
  }
  const std::unordered_map<std::string, CcfGroupPtr>& ccf_groups() const {
    return ccf_groups_;
  }
  /// @}

  /// Adds a fault tree into the model container.
  /// Fault trees are uniquely owned by this model.
  ///
  /// @param[in] fault_tree  A fault tree defined in this model.
  ///
  /// @throws RedefinitionError  The model has a container with the same name.
  void AddFaultTree(FaultTreePtr fault_tree);

  /// Adds a parameter that is used in this model's expressions.
  ///
  /// @param[in] parameter  A parameter defined in this model.
  ///
  /// @throws RedefinitionError  The model has a parameter with the same name.
  void AddParameter(const ParameterPtr& parameter);

  /// Adds a house event that is used in this model.
  ///
  /// @param[in] house_event  A house event defined in this model.
  ///
  /// @throws RedefinitionError  An event with the same name already exists.
  void AddHouseEvent(const HouseEventPtr& house_event);

  /// Adds a basic event that is used in this model.
  ///
  /// @param[in] basic_event  A basic event defined in this model.
  ///
  /// @throws RedefinitionError  An event with the same name already exists.
  void AddBasicEvent(const BasicEventPtr& basic_event);

  /// Adds a gate that is used in this model's fault trees or components.
  ///
  /// @param[in] gate  A gate defined in this model.
  ///
  /// @throws RedefinitionError  An event with the same name already exists.
  void AddGate(const GatePtr& gate);

  /// Adds a CCF group that is used in this model's fault trees.
  ///
  /// @param[in] ccf_group  A CCF group defined in this model.
  ///
  /// @throws RedefinitionError  The model has a CCF group with the same name.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

  /// Finds an entity (parameter, basic and house event, gate) from a reference.
  /// The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the entity found by following the given reference.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  /// @{
  ParameterPtr GetParameter(const std::string& reference,
                            const std::string& base_path);
  HouseEventPtr GetHouseEvent(const std::string& reference,
                              const std::string& base_path);
  BasicEventPtr GetBasicEvent(const std::string& reference,
                              const std::string& base_path);
  GatePtr GetGate(const std::string& reference, const std::string& base_path);
  /// @}

 private:
  /// Generic helper function to find an entity from a reference.
  /// The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @tparam Container  Map of name and entity pairs.
  ///
  /// @param[in] reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  /// @param[in] public_container  Model's container for public entities.
  /// @param[in] getter  The getter function to access
  ///                    the components' container of private entities.
  ///
  /// @returns Pointer to the requested entity.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  template <class Container>
  typename Container::mapped_type GetEntity(
      const std::string& reference,
      const std::string& base_path,
      const Container& public_container,
      const Container& (Component::*getter)() const);

  /// Helper function to find the container for references.
  ///
  /// @param[in] path  The ancestor container names chained with ".".
  ///
  /// @returns A fault tree or component from the path.
  ///
  /// @throws std::out_of_range  There's a missing container in the path.
  const Component& GetContainer(const std::vector<std::string>& path);

  /// A collection of defined constructs in the model.
  /// @{
  std::unordered_map<std::string, FaultTreePtr> fault_trees_;
  std::unordered_map<std::string, GatePtr> gates_;
  std::unordered_map<std::string, HouseEventPtr> house_events_;
  std::unordered_map<std::string, BasicEventPtr> basic_events_;
  std::unordered_map<std::string, ParameterPtr> parameters_;
  std::unordered_map<std::string, CcfGroupPtr> ccf_groups_;
  /// @}
  std::unordered_set<std::string> event_ids_;  ///< For faster lookup.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
