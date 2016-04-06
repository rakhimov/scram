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

/// @file model.cc
/// Implementation of functions in Model class.

#include "model.h"

#include <boost/algorithm/string.hpp>

namespace scram {

Model::Model(const std::string& name) : name_(name) {}

void Model::AddFaultTree(FaultTreePtr fault_tree) {
  std::string name = fault_tree->name();
  boost::to_lower(name);
  if (fault_trees_.count(name)) {
    std::string msg = "Redefinition of fault tree " + fault_tree->name();
    throw RedefinitionError(msg);
  }
  fault_trees_.emplace(name, std::move(fault_tree));
}

void Model::AddParameter(const ParameterPtr& parameter) {
  bool original = parameters_.emplace(parameter->id(), parameter).second;
  if (!original) {
    std::string msg = "Redefinition of parameter " + parameter->name();
    throw RedefinitionError(msg);
  }
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  std::string id = house_event->id();
  bool original = event_ids_.insert(id).second;
  if (!original) {
    std::string msg = "Redefinition of event " + house_event->name();
    throw RedefinitionError(msg);
  }
  house_events_.emplace(id, house_event);
}

void Model::AddBasicEvent(const BasicEventPtr& basic_event) {
  std::string id = basic_event->id();
  bool original = event_ids_.insert(id).second;
  if (!original) {
    std::string msg = "Redefinition of event " + basic_event->name();
    throw RedefinitionError(msg);
  }
  basic_events_.emplace(id, basic_event);
}

void Model::AddGate(const GatePtr& gate) {
  std::string id = gate->id();
  bool original = event_ids_.insert(id).second;
  if (!original) {
    std::string msg = "Redefinition of event " + gate->name();
    throw RedefinitionError(msg);
  }
  gates_.emplace(id, gate);
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->id();
  bool original = ccf_groups_.emplace(name, ccf_group).second;
  if (!original) {
    std::string msg = "Redefinition of CCF group " + ccf_group->name();
    throw RedefinitionError(msg);
  }
}

ParameterPtr Model::GetParameter(const std::string& reference,
                                 const std::string& base_path) {
  return Model::GetEntity(
      reference, base_path, parameters_,
      [](const Component& component) { return component.parameters(); });
}

HouseEventPtr Model::GetHouseEvent(const std::string& reference,
                                   const std::string& base_path) {
  return Model::GetEntity(
      reference, base_path, house_events_,
      [](const Component& component) { return component.house_events(); });
}

BasicEventPtr Model::GetBasicEvent(const std::string& reference,
                                   const std::string& base_path) {
  return Model::GetEntity(
      reference, base_path, basic_events_,
      [](const Component& component) { return component.basic_events(); });
}

GatePtr Model::GetGate(const std::string& reference,
                       const std::string& base_path) {
  return Model::GetEntity(
      reference, base_path, gates_,
      [](const Component& component) { return component.gates(); });
}

namespace {

/// @param[in] string_path  The reference string formatted with ".".
///
/// @returns A set of names in the reference path.
std::vector<std::string> GetPath(std::string string_path) {
  std::vector<std::string> path;
  boost::to_lower(string_path);
  boost::split(path, string_path, boost::is_any_of("."),
               boost::token_compress_on);
  return path;
}

}  // namespace

template <class Tptr, class Getter>
Tptr Model::GetEntity(
    const std::string& reference,
    const std::string& base_path,
    const std::unordered_map<std::string, Tptr>& public_container,
    Getter getter) {
  assert(!reference.empty());
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      return getter(Model::GetContainer(full_path)).at(target_name);
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) return public_container.at(target_name);  // Public entity.
  return getter(Model::GetContainer(path)).at(target_name);  // Direct access.
}

const Component& Model::GetContainer(const std::vector<std::string>& path) {
  assert(!path.empty());
  auto it = path.begin();  // The path starts with a fault tree.
  const Component* container = fault_trees_.at(*it).get();
  for (++it; it != path.end(); ++it) {
    container = container->components().at(*it).get();
  }
  return *container;
}

}  // namespace scram
