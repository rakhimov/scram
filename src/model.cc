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

const char Model::kDefaultName[] = "__unnamed-model__";

Model::Model(std::string name)
    : Element(name.empty() ? kDefaultName : std::move(name)),
      mission_time_(std::make_shared<MissionTime>()) {}

void Model::Add(InitiatingEventPtr initiating_event) {
  mef::AddElement<RedefinitionError>(std::move(initiating_event),
                                     &initiating_events_,
                                     "Redefinition of initiating event: ");
}

void Model::Add(EventTreePtr event_tree) {
  mef::AddElement<RedefinitionError>(std::move(event_tree), &event_trees_,
                                     "Redefinition of event tree: ");
}

void Model::Add(SequencePtr sequence) {
  mef::AddElement<RedefinitionError>(std::move(sequence), &sequences_,
                                     "Redefinition of sequence: ");
}

void Model::Add(RulePtr rule) {
  mef::AddElement<RedefinitionError>(std::move(rule), &rules_,
                                     "Redefinition of rule: ");
}

void Model::Add(FaultTreePtr fault_tree) {
  mef::AddElement<RedefinitionError>(std::move(fault_tree), &fault_trees_,
                                     "Redefinition of fault tree: ");
}

void Model::Add(ParameterPtr parameter) {
  mef::AddElement<RedefinitionError>(std::move(parameter), &parameters_,
                                     "Redefinition of parameter: ");
}

void Model::CheckDuplicateEvent(const Event& event) {
  const std::string& id = event.id();
  auto check = [&id](auto& table) { return table.entities_by_id.count(id); };
  if (check(gates_) || check(basic_events_) || check(house_events_))
    throw RedefinitionError("Redefinition of event: " + id);
}

void Model::Add(HouseEventPtr house_event) {
  CheckDuplicateEvent(*house_event);
  house_events_.insert(house_event);
}

void Model::Add(BasicEventPtr basic_event) {
  CheckDuplicateEvent(*basic_event);
  basic_events_.insert(basic_event);
}

void Model::Add(GatePtr gate) {
  CheckDuplicateEvent(*gate);
  gates_.insert(gate);
}

void Model::Add(CcfGroupPtr ccf_group) {
  mef::AddElement<RedefinitionError>(std::move(ccf_group), &ccf_groups_,
                                     "Redefinition of CCF group: ");
}

namespace {

/// Helper function to remove events from containers.
template <class T, class Table>
void RemoveEvent(T* event, Table* table) {
  auto it = table->find(event->id());
  if (it == table->end())
    throw std::out_of_range("Event " + event->id() + " is not in the model.");
  if (it->get() != event)
    throw std::out_of_range("Duplicate event " + event->id() +
                            " does not belong to the model.");
  table->erase(event);
}

}  // namespace

void Model::Remove(HouseEvent* house_event) {
  RemoveEvent(house_event, &house_events_);
}

void Model::Remove(BasicEvent* basic_event) {
  RemoveEvent(basic_event, &basic_events_);
}

void Model::Remove(Gate* gate) {
  RemoveEvent(gate, &gates_);
}

void Model::Remove(FaultTree* fault_tree) {
  auto it = fault_trees_.find(fault_tree->name());
  if (it == fault_trees_.end())
    throw std::out_of_range("Fault tree " + fault_tree->name() +
                            " is not in the model.");
  if (it->get() != fault_tree)
    throw std::out_of_range("Duplicate fault tree " + fault_tree->name() +
                            " does not belong to the model.");
  fault_trees_.erase(it);
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
