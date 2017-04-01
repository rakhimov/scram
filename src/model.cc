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

/// @file model.cc
/// Implementation of functions in Model class.

#include "model.h"

#include "error.h"
#include "ext/find_iterator.h"

namespace scram {
namespace mef {

const char Model::kDefaultName[] = "unnamed-model";

Model::Model(std::string name)
    : Element(name.empty() ? kDefaultName : std::move(name)),
      mission_time_(std::make_shared<MissionTime>()) {}

void Model::Add(EventTreePtr event_tree) {
  if (event_trees_.count(event_tree->name())) {
    throw RedefinitionError("Redefinition of event tree " + event_tree->name());
  }
  event_trees_.insert(std::move(event_tree));
}

void Model::Add(const SequencePtr& sequence) {
  if (sequences_.count(sequence->name())) {
    throw RedefinitionError("Redefinition of sequence " + sequence->name());
  }
  sequences_.insert(std::move(sequence));
}

void Model::Add(FaultTreePtr fault_tree) {
  if (fault_trees_.count(fault_tree->name())) {
    throw RedefinitionError("Redefinition of fault tree " + fault_tree->name());
  }
  fault_trees_.insert(std::move(fault_tree));
}

void Model::Add(const ParameterPtr& parameter) {
  if (!parameters_.Add(parameter)) {
    throw RedefinitionError("Redefinition of parameter " + parameter->name());
  }
}

void Model::Add(const HouseEventPtr& house_event) {
  bool original = events_.insert(house_event.get()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + house_event->name());
  }
  house_events_.Add(house_event);
}

void Model::Add(const BasicEventPtr& basic_event) {
  bool original = events_.insert(basic_event.get()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + basic_event->name());
  }
  basic_events_.Add(basic_event);
}

void Model::Add(const GatePtr& gate) {
  bool original = events_.insert(gate.get()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + gate->name());
  }
  gates_.Add(gate);
}

void Model::Add(const CcfGroupPtr& ccf_group) {
  if (ccf_groups_.insert(ccf_group).second == false) {
    throw RedefinitionError("Redefinition of CCF group " + ccf_group->name());
  }
}

Parameter* Model::GetParameter(const std::string& entity_reference,
                               const std::string& base_path) {
  return GetEntity(entity_reference, base_path, parameters_);
}

HouseEvent* Model::GetHouseEvent(const std::string& entity_reference,
                                 const std::string& base_path) {
  return GetEntity(entity_reference, base_path, house_events_);
}

BasicEvent* Model::GetBasicEvent(const std::string& entity_reference,
                                 const std::string& base_path) {
  return GetEntity(entity_reference, base_path, basic_events_);
}

Gate* Model::GetGate(const std::string& entity_reference,
                       const std::string& base_path) {
  return GetEntity(entity_reference, base_path, gates_);
}

template <class T>
T* Model::GetEntity(const std::string& entity_reference,
                    const std::string& base_path,
                    const LookupTable<T>& container) {
  assert(!entity_reference.empty());
  if (!base_path.empty()) {  // Check the local scope.
    if (auto it = ext::find(container.entities_by_path,
                            base_path + "." + entity_reference))
      return it->get();
  }

  auto at = [&entity_reference](const auto& reference_container) {
    if (auto it = ext::find(reference_container, entity_reference))
      return it->get();
    throw std::out_of_range("The event cannot be found.");
  };

  if (entity_reference.find('.') == std::string::npos)  // Public entity.
    return at(container.entities_by_id);

  return at(container.entities_by_path);  // Direct access.
}

/// Helper macro for Model::GetEvent event discovery.
#define GET_EVENT(access, path_reference)                          \
  do {                                                             \
    if (auto it = ext::find(gates_.access, path_reference))        \
      return it->get();                                            \
    if (auto it = ext::find(basic_events_.access, path_reference)) \
      return it->get();                                            \
    if (auto it = ext::find(house_events_.access, path_reference)) \
      return it->get();                                            \
  } while (false)

Formula::EventArg Model::GetEvent(const std::string& entity_reference,
                                  const std::string& base_path) {
  assert(!entity_reference.empty());
  if (!base_path.empty()) {  // Check the local scope.
    std::string full_path = base_path + "." + entity_reference;
    GET_EVENT(entities_by_path, full_path);
  }

  if (entity_reference.find('.') == std::string::npos) {  // Public entity.
    GET_EVENT(entities_by_id, entity_reference);
  } else {  // Direct access.
    GET_EVENT(entities_by_path, entity_reference);
  }
  throw std::out_of_range("The event cannot be bound.");
}

#undef GET_EVENT

}  // namespace mef
}  // namespace scram
