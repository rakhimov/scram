/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

#include "ccf_group.h"
#include "fault_tree.h"

namespace scram {

Model::Model(const std::string& name) : name_(name) {}

void Model::AddFaultTree(const FaultTreePtr& fault_tree) {
  std::string name = fault_tree->name();
  boost::to_lower(name);
  if (fault_trees_.count(name)) {
    std::string msg = "Redefinition of fault tree " + fault_tree->name();
    throw RedefinitionError(msg);
  }
  fault_trees_.insert(std::make_pair(name, fault_tree));
}

void Model::AddParameter(const ParameterPtr& parameter) {
  if (parameters_.count(parameter->id())) {
    std::string msg = "Redefinition of parameter " + parameter->name();
    throw RedefinitionError(msg);
  }
  parameters_.insert(std::make_pair(parameter->id(), parameter));
}

boost::shared_ptr<Parameter> Model::GetParameter(const std::string& reference,
                                                 const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    ComponentPtr scope = Model::GetContainer(base_path);
    ComponentPtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->parameters().count(target_name))
        return container->parameters().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, ParameterPtr>* parameters =
      &parameters_;
  if (path.size() > 1) {
    ComponentPtr container = Model::GetGlobalContainer(reference);
    parameters = &container->parameters();
  }

  if (parameters->count(target_name))
    return parameters->find(target_name)->second;

  std::string msg = "Undefined parameter " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

std::pair<boost::shared_ptr<Event>, std::string> Model::GetEvent(
    const std::string& reference,
    const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    ComponentPtr scope = Model::GetContainer(base_path);
    ComponentPtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->basic_events().count(target_name)) {
        EventPtr event = container->basic_events().find(target_name)->second;
        return std::make_pair(event, "basic-event");
      }

      if (container->gates().count(target_name)) {
        EventPtr event = container->gates().find(target_name)->second;
        return std::make_pair(event, "gate");
      }

      if (container->house_events().count(target_name)) {
        EventPtr event = container->house_events().find(target_name)->second;
        return std::make_pair(event, "house-event");
      }
    }
  }
  const boost::unordered_map<std::string, GatePtr>* gates = &gates_;
  const boost::unordered_map<std::string, HouseEventPtr>* house_events =
      &house_events_;
  const boost::unordered_map<std::string, BasicEventPtr>* basic_events =
      &basic_events_;
  if (path.size() > 1) {
    ComponentPtr container = Model::GetGlobalContainer(reference);
    gates = &container->gates();
    basic_events = &container->basic_events();
    house_events = &container->house_events();
  }

  if (basic_events->count(target_name)) {
    EventPtr event = basic_events->find(target_name)->second;
    return std::make_pair(event, "basic-event");
  }

  if (gates->count(target_name)) {
    EventPtr event = gates->find(target_name)->second;
    return std::make_pair(event, "gate");
  }

  if (house_events->count(target_name)) {
    EventPtr event = house_events->find(target_name)->second;
    return std::make_pair(event, "house-event");
  }

  std::string msg = "Undefined event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  std::string name = house_event->id();
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Redefinition of event " + house_event->name();
    throw RedefinitionError(msg);
  }
  house_events_.insert(std::make_pair(name, house_event));
}

boost::shared_ptr<HouseEvent> Model::GetHouseEvent(
    const std::string& reference,
    const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    ComponentPtr scope = Model::GetContainer(base_path);
    ComponentPtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->house_events().count(target_name))
        return container->house_events().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, HouseEventPtr>* house_events =
      &house_events_;
  if (path.size() > 1) {
    ComponentPtr container = Model::GetGlobalContainer(reference);
    house_events = &container->house_events();
  }

  if (house_events->count(target_name))
    return house_events->find(target_name)->second;

  std::string msg = "Undefined house event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddBasicEvent(const BasicEventPtr& basic_event) {
  std::string name = basic_event->id();
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Redefinition of event " + basic_event->name();
    throw RedefinitionError(msg);
  }
  basic_events_.insert(std::make_pair(name, basic_event));
}

boost::shared_ptr<BasicEvent> Model::GetBasicEvent(
    const std::string& reference,
    const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    ComponentPtr scope = Model::GetContainer(base_path);
    ComponentPtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->basic_events().count(target_name))
        return container->basic_events().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, BasicEventPtr>* basic_events =
      &basic_events_;
  if (path.size() > 1) {
    ComponentPtr container = Model::GetGlobalContainer(reference);
    basic_events = &container->basic_events();
  }

  if (basic_events->count(target_name))
    return basic_events->find(target_name)->second;

  std::string msg = "Undefined basic event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddGate(const GatePtr& gate) {
  std::string name = gate->id();
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Redefinition of event " + gate->name();
    throw RedefinitionError(msg);
  }
  gates_.insert(std::make_pair(name, gate));
}

boost::shared_ptr<Gate> Model::GetGate(const std::string& reference,
                                       const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    ComponentPtr scope = Model::GetContainer(base_path);
    ComponentPtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->gates().count(target_name))
        return container->gates().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, GatePtr>* gates = &gates_;
  if (path.size() > 1) {
    ComponentPtr container = Model::GetGlobalContainer(reference);
    gates = &container->gates();
  }

  if (gates->count(target_name))
    return gates->find(target_name)->second;

  std::string msg = "Undefined gate " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->id();
  if (ccf_groups_.count(name)) {
    std::string msg = "Redefinition of CCF group " + ccf_group->name();
    throw RedefinitionError(msg);
  }
  ccf_groups_.insert(std::make_pair(name, ccf_group));
}

boost::shared_ptr<Component> Model::GetContainer(const std::string& base_path) {
  assert(base_path != "");
  std::vector<std::string> path;
  boost::split(path, base_path, boost::is_any_of("."),
               boost::token_compress_on);
  std::vector<std::string>::iterator it = path.begin();
  std::string name = *it;
  boost::to_lower(name);
  if (!fault_trees_.count(name)) throw LogicError("Missing fault tree " + *it);
  ComponentPtr container = fault_trees_.find(name)->second;
  const boost::unordered_map<std::string, ComponentPtr>* candidates;
  for (++it; it != path.end(); ++it) {
    name = *it;
    boost::to_lower(name);
    candidates = &container->components();
    if (!candidates->count(name)) {
      throw LogicError("Undefined component " + *it + " in path " + base_path);
    }
    container = candidates->find(name)->second;
  }
  return container;
}

boost::shared_ptr<Component> Model::GetLocalContainer(
    const std::string& reference,
    const ComponentPtr& scope) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  ComponentPtr container = scope;
  if (path.size() > 1) {
    const boost::unordered_map<std::string, ComponentPtr>* candidates;
    for (int i = 0; i < path.size() - 1; ++i) {
      std::string name = path[i];
      boost::to_lower(name);
      candidates = &container->components();
      if (!candidates->count(name)) {  // No container available.
        ComponentPtr undefined;
        return undefined;  // Not possible to reach locally.
      }
      container = candidates->find(name)->second;
    }
  }
  return container;
}

boost::shared_ptr<Component> Model::GetGlobalContainer(
    const std::string& reference) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  assert(path.size() > 1);
  std::string name = path.front();
  boost::to_lower(name);
  if (!fault_trees_.count(name)) {
    throw ValidationError("Undefined fault tree " + path.front() +
                          " in reference " + reference);
  }
  ComponentPtr container = fault_trees_.find(name)->second;
  const boost::unordered_map<std::string, ComponentPtr>* candidates;
  for (int i = 1; i < path.size() - 1; ++i) {
    std::string name = path[i];
    boost::to_lower(name);
    candidates = &container->components();
    if (!candidates->count(name)) {  // No container available.
      throw ValidationError("Undefined component " + path[i] +
                            " in reference " + reference);
    }
    container = candidates->find(name)->second;
  }
  return container;
}

}  // namespace scram
