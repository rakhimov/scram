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
#include <type_traits>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
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
  /// @todo Only Model is allowed to have an optional name,
  ///       while all other Elements require names.
  ///       An empty name is an error for Element class invariants as well.
  ///       This leads to a nasty magic string based optional name for a model.
  static const char kDefaultName[];

  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  ///
  /// @throws InvalidArgument  The name is malformed.
  explicit Model(std::string name = "");

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

  /// Adds MEF constructs into the model container.
  ///
  /// @param[in] element  An element defined in this model.
  ///
  /// @throws RedefinitionError  The element is already defined in the model.
  ///
  /// @{
  void Add(InitiatingEventPtr element);
  void Add(EventTreePtr element);
  void Add(const SequencePtr& element);
  void Add(RulePtr element);
  void Add(FaultTreePtr element);
  void Add(const ParameterPtr& element);
  void Add(const HouseEventPtr& element);
  void Add(const BasicEventPtr& element);
  void Add(const GatePtr& element);
  void Add(const CcfGroupPtr& element);
  void Add(std::unique_ptr<Expression> element) {
    expressions_.emplace_back(std::move(element));
  }
  void Add(std::unique_ptr<Instruction> element) {
    instructions_.emplace_back(std::move(element));
  }
  /// @}

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
  Parameter* GetParameter(const std::string& entity_reference,
                          const std::string& base_path);
  HouseEvent* GetHouseEvent(const std::string& entity_reference,
                              const std::string& base_path);
  BasicEvent* GetBasicEvent(const std::string& entity_reference,
                              const std::string& base_path);
  Gate* GetGate(const std::string& entity_reference,
                const std::string& base_path);
  Formula::EventArg
  GetEvent(const std::string& entity_reference, const std::string& base_path);
  /// @}

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
    /// @returns The result of insert call to IdTable.
    auto insert(const std::shared_ptr<T>& entity) {
      auto it = entities_by_id.insert(entity);
      if (it.second)
        entities_by_path.insert(entity);
      return it;
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
  T* GetEntity(const std::string& entity_reference,
               const std::string& base_path, const LookupTable<T>& container);

  /// A collection of defined constructs in the model.
  /// @{
  ElementTable<InitiatingEventPtr> initiating_events_;
  ElementTable<EventTreePtr> event_trees_;
  ElementTable<SequencePtr> sequences_;
  ElementTable<RulePtr> rules_;
  ElementTable<FaultTreePtr> fault_trees_;
  LookupTable<Gate> gates_;
  LookupTable<HouseEvent> house_events_;
  LookupTable<BasicEvent> basic_events_;
  LookupTable<Parameter> parameters_;
  std::shared_ptr<MissionTime> mission_time_;
  IdTable<CcfGroupPtr> ccf_groups_;
  std::vector<std::unique_ptr<Expression>> expressions_;
  std::vector<std::unique_ptr<Instruction>> instructions_;
  /// @}
  IdTable<Event*> events_;  ///< All events by ids.
  Context context_;  ///< The context to be used by test-event expressions.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
