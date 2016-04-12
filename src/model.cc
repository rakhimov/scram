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

Model::Model(std::string name) : name_(std::move(name)) {}

void Model::AddFaultTree(FaultTreePtr fault_tree) {
  if (fault_trees_.count(fault_tree->name())) {
    throw RedefinitionError("Redefinition of fault tree " + fault_tree->name());
  }
  fault_trees_.emplace(fault_tree->name(), std::move(fault_tree));
}

void Model::AddParameter(const ParameterPtr& parameter) {
  bool original = parameters_.emplace(parameter->id(), parameter).second;
  if (!original) {
    throw RedefinitionError("Redefinition of parameter " + parameter->name());
  }
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  bool original = event_ids_.insert(house_event->id()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + house_event->name());
  }
  house_events_.emplace(house_event->id(), house_event);
}

void Model::AddBasicEvent(const BasicEventPtr& basic_event) {
  bool original = event_ids_.insert(basic_event->id()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + basic_event->name());
  }
  basic_events_.emplace(basic_event->id(), basic_event);
}

void Model::AddGate(const GatePtr& gate) {
  bool original = event_ids_.insert(gate->id()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + gate->name());
  }
  gates_.emplace(gate->id(), gate);
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  bool original = ccf_groups_.emplace(ccf_group->id(), ccf_group).second;
  if (!original) {
    throw RedefinitionError("Redefinition of CCF group " + ccf_group->name());
  }
}

ParameterPtr Model::GetParameter(const std::string& reference,
                                 const std::string& base_path) {
  return Model::GetEntity(reference, base_path, parameters_,
                          &Component::parameters);
}

HouseEventPtr Model::GetHouseEvent(const std::string& reference,
                                   const std::string& base_path) {
  return Model::GetEntity(reference, base_path, house_events_,
                          &Component::house_events);
}

BasicEventPtr Model::GetBasicEvent(const std::string& reference,
                                   const std::string& base_path) {
  return Model::GetEntity(reference, base_path, basic_events_,
                          &Component::basic_events);
}

GatePtr Model::GetGate(const std::string& reference,
                       const std::string& base_path) {
  return Model::GetEntity(reference, base_path, gates_, &Component::gates);
}

namespace {

/// @param[in] string_path  The reference string formatted with ".".
///
/// @returns A set of names in the reference path.
std::vector<std::string> GetPath(std::string string_path) {
  std::vector<std::string> path;
  return boost::split(path, string_path, boost::is_any_of("."));
}

}  // namespace

template <class Container>
typename Container::mapped_type Model::GetEntity(
    const std::string& reference,
    const std::string& base_path,
    const Container& public_container,
    const Container& (Component::*getter)() const) {
  assert(!reference.empty());
  std::vector<std::string> path = GetPath(reference);
  std::string target_name = path.back();
  path.pop_back();
  if (!base_path.empty()) {  // Check the local scope.
    std::vector<std::string> full_path = GetPath(base_path);
    full_path.insert(full_path.end(), path.begin(), path.end());
    try {
      return (Model::GetContainer(full_path).*getter)().at(target_name);
    } catch (std::out_of_range&) {}  // Continue searching.
  }
  if (path.empty()) return public_container.at(target_name);  // Public entity.
  return (Model::GetContainer(path).*getter)().at(target_name);  // Direct call.
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
