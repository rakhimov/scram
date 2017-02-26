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
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/noncopyable.hpp>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "fault_tree.h"
#include "parameter.h"

namespace scram {
namespace mef {

/// This class represents a risk analysis model.
class Model : public Element, private boost::noncopyable {
 public:
  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  ///
  /// @throws InvalidArgument  The name is malformed.
  explicit Model(std::string name = "");

  /// @returns Defined constructs in the model.
  /// @{
  const ElementTable<FaultTreePtr>& fault_trees() const { return fault_trees_; }
  const IdTable<ParameterPtr>& parameters() const {
    return parameters_.entities_by_id;
  }
  const std::shared_ptr<MissionTime>& mission_time() const {
    return mission_time_;
  }
  const IdTable<HouseEventPtr>& house_events() const {
    return house_events_.entities_by_id;
  }
  const IdTable<BasicEventPtr>& basic_events() const {
    return basic_events_.entities_by_id;
  }
  const IdTable<GatePtr>& gates() const { return gates_.entities_by_id; }
  const IdTable<CcfGroupPtr>& ccf_groups() const { return ccf_groups_; }
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
                            const std::string& base_path);
  HouseEventPtr GetHouseEvent(const std::string& entity_reference,
                              const std::string& base_path);
  BasicEventPtr GetBasicEvent(const std::string& entity_reference,
                              const std::string& base_path);
  GatePtr GetGate(const std::string& entity_reference,
                  const std::string& base_path);
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
  struct LookupTable {
    static_assert(std::is_base_of<Role, T>::value, "Entity without a role!");

    /// Container with full path to elements.
    ///
    /// @tparam Ptr  Pointer type to the T.
    template <typename Ptr>
    using PathTable = boost::multi_index_container<
        Ptr,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<boost::multi_index::global_fun<
                const Ptr&, std::string, &GetFullPath>>>>;

    /// Adds an entry with an entity into lookup containers.
    ///
    /// @param[in] entity  The candidate entity.
    ///
    /// @returns false if the entity is duplicate and hasn't been added.
    bool Add(const std::shared_ptr<T>& entity) {
      if (entities_by_id.insert(entity).second == false)
        return false;

      entities_by_path.insert(entity);
      return true;
    }

    IdTable<std::shared_ptr<T>> entities_by_id;  ///< Entity id as a key.
    PathTable<std::shared_ptr<T>> entities_by_path;  ///< Full path as a key.
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
                               const LookupTable<T>& container);

  /// A collection of defined constructs in the model.
  /// @{
  ElementTable<FaultTreePtr> fault_trees_;
  LookupTable<Gate> gates_;
  LookupTable<HouseEvent> house_events_;
  LookupTable<BasicEvent> basic_events_;
  LookupTable<Parameter> parameters_;
  std::shared_ptr<MissionTime> mission_time_;
  IdTable<CcfGroupPtr> ccf_groups_;
  /// @}
  IdTable<Event*> events_;  ///< All events by ids.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
