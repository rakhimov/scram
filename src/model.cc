/// @file model.cc
/// Implementation of functions in Model class.
#include "model.h"

#include "ccf_group.h"
#include "fault_tree.h"

#include <boost/algorithm/string.hpp>

namespace scram {

Model::Model(std::string name) : name_(name) {}

void Model::AddFaultTree(const FaultTreePtr& fault_tree) {
  std::string name = fault_tree->name();
  boost::to_lower(name);
  if (fault_trees_.count(name)) {
    std::string msg = "Fault tree " + fault_tree->name() + " already exists.";
    throw ValidationError(msg);
  }
  fault_trees_.insert(std::make_pair(name, fault_tree));
}

void Model::AddParameter(const ParameterPtr& parameter) {
  if (parameters_.count(parameter->id())) {
    std::string msg = "Parameter " + parameter->name() + " already exists.";
    throw ValidationError(msg);
  }
  parameters_.insert(std::make_pair(parameter->id(), parameter));
}

boost::shared_ptr<Parameter> Model::GetParameter(const std::string& reference) {
  std::string id = reference;
  boost::to_lower(id);
  if (parameters_.count(id)) {
    return parameters_.find(id)->second;

  } else {
    std::string msg = "Undefined parameter: " + reference;
    throw ValidationError(msg);
  }
}

boost::shared_ptr<Event> Model::GetEvent(const std::string& reference,
                                         const std::string& base_path) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  std::string target_name = path.back();
  boost::to_lower(target_name);
  if (base_path != "") {
    FaultTreePtr scope = Model::GetContainer(base_path);
    FaultTreePtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->basic_events().count(target_name))
        return container->basic_events().find(target_name)->second;

      if (container->gates().count(target_name))
        return container->gates().find(target_name)->second;

      if (container->house_events().count(target_name))
        return container->house_events().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, GatePtr>* gates = &gates_;
  const boost::unordered_map<std::string, HouseEventPtr>* house_events =
      &house_events_;
  const boost::unordered_map<std::string, BasicEventPtr>* basic_events =
      &basic_events_;
  if (path.size() > 1) {
    FaultTreePtr container = Model::GetGlobalContainer(reference);
    gates = &container->gates();
    basic_events = &container->basic_events();
    house_events = &container->house_events();
  }

  if (basic_events->count(target_name))
    return basic_events->find(target_name)->second;

  if (gates->count(target_name))
    return gates->find(target_name)->second;

  if (house_events->count(target_name))
    return house_events->find(target_name)->second;

  std::string msg = "Undefined event " + path.back() + " in reference " +
                    reference + " with base path " + base_path;
  throw ValidationError(msg);
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  std::string name = house_event->id();
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Event " + house_event->name() + " already exists.";
    throw ValidationError(msg);
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
    FaultTreePtr scope = Model::GetContainer(base_path);
    FaultTreePtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->house_events().count(target_name))
        return container->house_events().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, HouseEventPtr>* house_events =
      &house_events_;
  if (path.size() > 1) {
    FaultTreePtr container = Model::GetGlobalContainer(reference);
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
    std::string msg = "Event " + basic_event->name() + " already exists.";
    throw ValidationError(msg);
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
    FaultTreePtr scope = Model::GetContainer(base_path);
    FaultTreePtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->basic_events().count(target_name))
        return container->basic_events().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, BasicEventPtr>* basic_events =
      &basic_events_;
  if (path.size() > 1) {
    FaultTreePtr container = Model::GetGlobalContainer(reference);
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
    std::string msg = "Event " + gate->name() + " already exists.";
    throw ValidationError(msg);
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
    FaultTreePtr scope = Model::GetContainer(base_path);
    FaultTreePtr container = Model::GetLocalContainer(reference, scope);
    if (container) {
      if (container->gates().count(target_name))
        return container->gates().find(target_name)->second;
    }
  }
  const boost::unordered_map<std::string, GatePtr>* gates = &gates_;
  if (path.size() > 1) {
    FaultTreePtr container = Model::GetGlobalContainer(reference);
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
    std::string msg = "CCF group " + ccf_group->name() + " already exists.";
    throw ValidationError(msg);
  }
  ccf_groups_.insert(std::make_pair(name, ccf_group));
}

boost::shared_ptr<FaultTree> Model::GetContainer(const std::string& base_path) {
  assert(base_path != "");
  std::vector<std::string> path;
  boost::split(path, base_path, boost::is_any_of("."),
               boost::token_compress_on);
  std::vector<std::string>::iterator it = path.begin();
  std::string name = *it;
  boost::to_lower(name);
  if (!fault_trees_.count(name)) throw LogicError("Missing fault tree " + *it);
  FaultTreePtr container = fault_trees_.find(name)->second;
  const boost::unordered_map<std::string, ComponentPtr>* candidates;
  for(++it; it != path.end(); ++it) {
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

boost::shared_ptr<FaultTree> Model::GetLocalContainer(
    const std::string& reference,
    const FaultTreePtr& scope) {
  assert(reference != "");
  std::vector<std::string> path;
  boost::split(path, reference, boost::is_any_of("."),
               boost::token_compress_on);
  FaultTreePtr container = scope;
  if (path.size() > 1) {
    const boost::unordered_map<std::string, ComponentPtr>* candidates;
    for (int i = 0; i < path.size() - 1; ++i) {
      std::string name = path[i];
      boost::to_lower(name);
      candidates = &container->components();
      if (!candidates->count(name)) {  // No container available.
        FaultTreePtr undefined;
        return undefined;  // Not possible to reach locally.
      }
      container = candidates->find(name)->second;
    }
  }
  return container;
}

boost::shared_ptr<FaultTree> Model::GetGlobalContainer(
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
  FaultTreePtr container = fault_trees_.find(name)->second;
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
