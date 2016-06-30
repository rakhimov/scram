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
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "ext.h"
#include "expression.h"
#include "fault_tree.h"

namespace scram {
namespace mef {

/// This class represents a risk analysis model.
class Model : public Element {
 public:
  /// Table for constructs in the model identified by names or ids.
  ///
  /// @tparam T  The type of a stored value.
  template <typename T>
  using Table = std::unordered_map<std::string, T>;

  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  ///
  /// @throws InvalidArgument  The name is malformed.
  explicit Model(std::string name = "");

  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

  /// @returns Defined constructs in the model.
  /// @{
  const Table<FaultTreePtr>& fault_trees() const { return fault_trees_; }
  const Table<ParameterPtr>& parameters() const {
    return parameters_.entities_by_id;
  }
  const Table<HouseEventPtr>& house_events() const {
    return house_events_.entities_by_id;
  }
  const Table<BasicEventPtr>& basic_events() const {
    return basic_events_.entities_by_id;
  }
  const Table<GatePtr>& gates() const { return gates_.entities_by_id; }
  const Table<CcfGroupPtr>& ccf_groups() const { return ccf_groups_; }
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
  /// The reference is case sensitive
  /// and can contain an identifier, full path, or local path.
  ///
  /// @param[in] entity_reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the entity found by following the given reference.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  /// @{
  ParameterPtr GetParameter(const std::string& entity_reference,
                            const std::string& base_path) {
    return Model::GetEntity(entity_reference, base_path, parameters_);
  }
  HouseEventPtr GetHouseEvent(const std::string& entity_reference,
                              const std::string& base_path) {
    return Model::GetEntity(entity_reference, base_path, house_events_);
  }
  BasicEventPtr GetBasicEvent(const std::string& entity_reference,
                              const std::string& base_path) {
    return Model::GetEntity(entity_reference, base_path, basic_events_);
  }
  GatePtr GetGate(const std::string& entity_reference,
                  const std::string& base_path) {
    return Model::GetEntity(entity_reference, base_path, gates_);
  }
  /// @}

  /// Binds a formula with its argument event.
  /// This is a special handling for undefined event types
  /// with mixed roles.
  ///
  /// @param[in] entity_reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  /// @param[out] formula  The host formula for the event.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  void BindEvent(const std::string& entity_reference,
                 const std::string& base_path, Formula* formula);

 private:
  /// Lookup containers for model entities with roles.
  ///
  /// @tparam T  Type of an entity with a role.
  template <typename T>
  struct Lookup {
    static_assert(std::is_base_of<Role, T>::value, "Entity without a role!");

    /// Adds an entry with an entity into lookup containers.
    ///
    /// @param[in] entity  The candidate entity.
    ///
    /// @returns false if the entity is duplicate and hasn't been added.
    bool Add(const std::shared_ptr<T>& entity) {
      if (entities_by_id.emplace(entity->id(), entity).second == false)
        return false;

      entities_by_path.emplace(entity->base_path() + "." + entity->name(),
                               entity);
      return true;
    }

    Table<std::shared_ptr<T>> entities_by_id;  ///< Entity id as a key.
    Table<std::shared_ptr<T>> entities_by_path;  ///< Full path as a key.
  };

  /// Generic helper function to find an entity from a reference.
  /// The reference is case sensitive
  /// and can contain an identifier, full path, or local path.
  ///
  /// @tparam Container  Map of name and entity pairs.
  ///
  /// @param[in] entity_reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  /// @param[in] container  Model's lookup container for entities.
  ///
  /// @returns Pointer to the requested entity.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  template <class T>
  std::shared_ptr<T> GetEntity(const std::string& entity_reference,
                               const std::string& base_path,
                               const Lookup<T>& container) {
    assert(!entity_reference.empty());
    if (!base_path.empty()) {  // Check the local scope.
      if (auto it = ext::find(container.entities_by_path,
                              base_path + "." + entity_reference))
        return it->second;
    }

    if (entity_reference.find('.') == std::string::npos)  // Public entity.
      return container.entities_by_id.at(entity_reference);

    return container.entities_by_path.at(entity_reference);  // Direct access.
  }

  /// A collection of defined constructs in the model.
  /// @{
  Table<FaultTreePtr> fault_trees_;
  Lookup<Gate> gates_;
  Lookup<HouseEvent> house_events_;
  Lookup<BasicEvent> basic_events_;
  Lookup<Parameter> parameters_;
  Table<CcfGroupPtr> ccf_groups_;
  /// @}
  std::unordered_set<std::string> event_ids_;  ///< All event ids.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
