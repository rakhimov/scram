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
  bool original = parameters_.insert({parameter->id(), parameter}).second;
  if (!original) {
    std::string msg = "Redefinition of parameter " + parameter->name();
    throw RedefinitionError(msg);
  }
}

std::shared_ptr<Parameter> Model::GetParameter(const std::string& reference,
                                               const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    const Component* scope = Model::GetContainer(base_path);
    const Component* container = Model::GetLocalContainer(reference, scope);
    if (container) {
      try {
        return container->parameters().at(target_name);
      } catch (std::out_of_range& err) {}  // Continue searching.
    }
  }
  const std::unordered_map<std::string, ParameterPtr>* parameters =
      &parameters_;
  if (path.size() > 1) {
    const Component* container = Model::GetGlobalContainer(reference);
    parameters = &container->parameters();
  }

  try {
    return parameters->at(target_name);
  } catch (std::out_of_range& err) {}

  std::string msg = "Undefined parameter " + path.back() + " in reference " +
      reference + " with base path " + base_path;
  throw ValidationError(msg);
}

std::pair<std::shared_ptr<Event>, std::string> Model::GetEvent(
    const std::string& reference,
    const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    const Component* scope = Model::GetContainer(base_path);
    const Component* container = Model::GetLocalContainer(reference, scope);
    if (container) {
      try {
        EventPtr event = container->basic_events().at(target_name);
        return {event, "basic-event"};
      } catch (std::out_of_range& err) {}

      try {
        EventPtr event = container->gates().at(target_name);
        return {event, "gate"};
      } catch (std::out_of_range& err) {}

      try {
        EventPtr event = container->house_events().at(target_name);
        return {event, "house-event"};
      } catch (std::out_of_range& err) {}
    }
  }
  const std::unordered_map<std::string, GatePtr>* gates = &gates_;
  const std::unordered_map<std::string, HouseEventPtr>* house_events =
      &house_events_;
  const std::unordered_map<std::string, BasicEventPtr>* basic_events =
      &basic_events_;
  if (path.size() > 1) {
    const Component* container = Model::GetGlobalContainer(reference);
    gates = &container->gates();
    basic_events = &container->basic_events();
    house_events = &container->house_events();
  }

  try {
    EventPtr event = basic_events->at(target_name);
    return {event, "basic-event"};
  } catch (std::out_of_range& err) {}

  try {
    EventPtr event = gates->at(target_name);
    return {event, "gate"};
  } catch (std::out_of_range& err) {}

  try {
    EventPtr event = house_events->at(target_name);
    return {event, "house-event"};
  } catch (std::out_of_range& err) {}

  std::string msg = "Undefined event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  std::string id = house_event->id();
  bool original = event_ids_.insert(id).second;
  if (!original) {
    std::string msg = "Redefinition of event " + house_event->name();
    throw RedefinitionError(msg);
  }
  house_events_.insert({id, house_event});
}

std::shared_ptr<HouseEvent> Model::GetHouseEvent(const std::string& reference,
                                                 const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    const Component* scope = Model::GetContainer(base_path);
    const Component* container = Model::GetLocalContainer(reference, scope);
    if (container) {
      try {
        return container->house_events().at(target_name);
      } catch (std::out_of_range& err) {}  // Continue searching.
    }
  }
  const std::unordered_map<std::string, HouseEventPtr>* house_events =
      &house_events_;
  if (path.size() > 1) {
    const Component* container = Model::GetGlobalContainer(reference);
    house_events = &container->house_events();
  }

  try {
    return house_events->at(target_name);
  } catch (std::out_of_range& err) {}

  std::string msg = "Undefined house event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddBasicEvent(const BasicEventPtr& basic_event) {
  std::string id = basic_event->id();
  bool original = event_ids_.insert(id).second;
  if (!original) {
    std::string msg = "Redefinition of event " + basic_event->name();
    throw RedefinitionError(msg);
  }
  basic_events_.insert({id, basic_event});
}

std::shared_ptr<BasicEvent> Model::GetBasicEvent(const std::string& reference,
                                                 const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    const Component* scope = Model::GetContainer(base_path);
    const Component* container = Model::GetLocalContainer(reference, scope);
    if (container) {
      try {
        return container->basic_events().at(target_name);
      } catch (std::out_of_range& err) {}  // Continue searching.
    }
  }
  const std::unordered_map<std::string, BasicEventPtr>* basic_events =
      &basic_events_;
  if (path.size() > 1) {
    const Component* container = Model::GetGlobalContainer(reference);
    basic_events = &container->basic_events();
  }

  try {
    return basic_events->at(target_name);
  } catch (std::out_of_range& err) {}

  std::string msg = "Undefined basic event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddGate(const GatePtr& gate) {
  std::string id = gate->id();
  bool original = event_ids_.insert(id).second;
  if (!original) {
    std::string msg = "Redefinition of event " + gate->name();
    throw RedefinitionError(msg);
  }
  gates_.insert({id, gate});
}

std::shared_ptr<Gate> Model::GetGate(const std::string& reference,
                                     const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    const Component* scope = Model::GetContainer(base_path);
    const Component* container = Model::GetLocalContainer(reference, scope);
    if (container) {
      try {
        return container->gates().at(target_name);
      } catch (std::out_of_range& err) {}  // Continue searching.
    }
  }
  const std::unordered_map<std::string, GatePtr>* gates = &gates_;
  if (path.size() > 1) {
    const Component* container = Model::GetGlobalContainer(reference);
    gates = &container->gates();
  }

  try {
    return gates->at(target_name);
  } catch (std::out_of_range& err) {}  // Continue searching.

  std::string msg = "Undefined gate " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->id();
  bool original = ccf_groups_.insert({name, ccf_group}).second;
  if (!original) {
    std::string msg = "Redefinition of CCF group " + ccf_group->name();
    throw RedefinitionError(msg);
  }
}

const Component* Model::GetContainer(const std::string& base_path) {
  assert(base_path != "");
  std::vector<std::string> path;
  boost::split(path, base_path, boost::is_any_of("."),
               boost::token_compress_on);
  std::vector<std::string>::iterator it = path.begin();
  std::string name = *it;
  boost::to_lower(name);
  const Component* container;
  try {
    container = fault_trees_.at(name).get();
  } catch (std::out_of_range& err) {
    throw LogicError("Missing fault tree " + *it);
  }
  for (++it; it != path.end(); ++it) {
    name = *it;
    boost::to_lower(name);
    try {
      container = container->components().at(name).get();
    } catch (std::out_of_range& err) {
      throw LogicError("Undefined component " + *it + " in path " + base_path);
    }
  }
  return container;
}

const Component* Model::GetLocalContainer(const std::string& reference,
                                          const Component* scope) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  const Component* container = scope;
  if (path.size() > 1) {
    for (int i = 0; i < path.size() - 1; ++i) {
      std::string name = path[i];
      boost::to_lower(name);
      try {
        container = container->components().at(name).get();
      } catch (std::out_of_range& err) {
        return nullptr;  // Not possible to reach locally.
      }
    }
  }
  return container;
}

const Component* Model::GetGlobalContainer(const std::string& reference) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  assert(path.size() > 1);
  std::string name = path.front();
  boost::to_lower(name);
  const Component* container;
  try {
    container = fault_trees_.at(name).get();
  } catch (std::out_of_range& err) {
    throw ValidationError("Undefined fault tree " + path.front() +
                          " in reference " + reference);
  }
  for (int i = 1; i < path.size() - 1; ++i) {
    std::string name = path[i];
    boost::to_lower(name);
    try {
      container = container->components().at(name).get();
    } catch (std::out_of_range& err) {
      throw ValidationError("Undefined component " + path[i] +
                            " in reference " + reference);
    }
  }
  return container;
}

}  // namespace scram
