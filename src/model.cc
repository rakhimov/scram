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

namespace scram {
namespace mef {

Model::Model(std::string name) : Element(std::move(name), /*optional=*/true) {}

void Model::AddFaultTree(FaultTreePtr fault_tree) {
  if (fault_trees_.count(fault_tree->name())) {
    throw RedefinitionError("Redefinition of fault tree " + fault_tree->name());
  }
  fault_trees_.emplace(fault_tree->name(), std::move(fault_tree));
}

void Model::AddParameter(const ParameterPtr& parameter) {
  if (!parameters_.Add(parameter)) {
    throw RedefinitionError("Redefinition of parameter " + parameter->name());
  }
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  bool original = event_ids_.insert(house_event->id()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + house_event->name());
  }
  house_events_.Add(house_event);
}

void Model::AddBasicEvent(const BasicEventPtr& basic_event) {
  bool original = event_ids_.insert(basic_event->id()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + basic_event->name());
  }
  basic_events_.Add(basic_event);
}

void Model::AddGate(const GatePtr& gate) {
  bool original = event_ids_.insert(gate->id()).second;
  if (!original) {
    throw RedefinitionError("Redefinition of event " + gate->name());
  }
  gates_.Add(gate);
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  bool original = ccf_groups_.emplace(ccf_group->id(), ccf_group).second;
  if (!original) {
    throw RedefinitionError("Redefinition of CCF group " + ccf_group->name());
  }
}

/// Helper macro for Model::BindEvent event discovery.
#define BIND_EVENT(access, path_reference)                       \
  if (auto it = ext::find(gates_.access, path_reference))        \
    return formula->AddArgument(it->second);                     \
  if (auto it = ext::find(basic_events_.access, path_reference)) \
    return formula->AddArgument(it->second);                     \
  if (auto it = ext::find(house_events_.access, path_reference)) \
    return formula->AddArgument(it->second)

void Model::BindEvent(const std::string& entity_reference,
                      const std::string& base_path, Formula* formula) {
  assert(!entity_reference.empty());
  if (!base_path.empty()) {  // Check the local scope.
    std::string full_path = base_path + "." + entity_reference;
    BIND_EVENT(entities_by_path, full_path);
  }

  if (entity_reference.find('.') == std::string::npos) {  // Public entity.
    BIND_EVENT(entities_by_id, entity_reference);
  } else {  // Direct access.
    BIND_EVENT(entities_by_path, entity_reference);
  }
  throw std::out_of_range("The event cannot be bound.");
}

#undef BIND_EVENT

}  // namespace mef
}  // namespace scram
