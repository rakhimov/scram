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

#include <vector>

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

ParameterPtr Model::GetParameter(const std::string& reference,
                                 const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      return Model::GetContainer(full_path).parameters().at(target_name);
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) return parameters_.at(target_name);  // Public parameter.
  return Model::GetContainer(path).parameters().at(target_name);
}

std::pair<EventPtr, std::string> Model::GetEvent(const std::string& reference,
                                                 const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      const Component& container = Model::GetContainer(full_path);
      try {
        EventPtr event = container.basic_events().at(target_name);
        return {event, "basic-event"};
      } catch (std::out_of_range&) {}

      try {
        EventPtr event = container.gates().at(target_name);
        return {event, "gate"};
      } catch (std::out_of_range&) {}

      EventPtr event = container.house_events().at(target_name);
      return {event, "house-event"};
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) {  // Public event.
    try {
      EventPtr event = basic_events_.at(target_name);
      return {event, "basic-event"};
    } catch (std::out_of_range&) {}

    try {
      EventPtr event = gates_.at(target_name);
      return {event, "gate"};
    } catch (std::out_of_range&) {}

    EventPtr event = house_events_.at(target_name);
    return {event, "house-event"};
  }
  const Component& container = Model::GetContainer(path);
  try {
    EventPtr event = container.basic_events().at(target_name);
    return {event, "basic-event"};
  } catch (std::out_of_range&) {}

  try {
    EventPtr event = container.gates().at(target_name);
    return {event, "gate"};
  } catch (std::out_of_range&) {}

  EventPtr event = container.house_events().at(target_name);
  return {event, "house-event"};
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

HouseEventPtr Model::GetHouseEvent(const std::string& reference,
                                   const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      return Model::GetContainer(full_path).house_events().at(target_name);
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) return house_events_.at(target_name);  // Public entity.
  return Model::GetContainer(path).house_events().at(target_name);
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

BasicEventPtr Model::GetBasicEvent(const std::string& reference,
                                   const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      return Model::GetContainer(full_path).basic_events().at(target_name);
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) return basic_events_.at(target_name);  // Public entity.
  return Model::GetContainer(path).basic_events().at(target_name);
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

GatePtr Model::GetGate(const std::string& reference,
                       const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      return Model::GetContainer(full_path).gates().at(target_name);
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) return gates_.at(target_name);  // Public entity.
  return Model::GetContainer(path).gates().at(target_name);
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->id();
  bool original = ccf_groups_.emplace(name, ccf_group).second;
  if (!original) {
    std::string msg = "Redefinition of CCF group " + ccf_group->name();
    throw RedefinitionError(msg);
  }
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
