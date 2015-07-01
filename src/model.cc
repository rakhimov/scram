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
  std::string name = parameter->name();
  boost::to_lower(name);

  if (parameters_.count(name)) {
    std::string msg = "Parameter " + parameter->name() + " already exists.";
    throw ValidationError(msg);
  }
  parameters_.insert(std::make_pair(name, parameter));
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

boost::shared_ptr<Event> Model::GetEvent(const std::string& reference) {
  std::string id = reference;
  boost::to_lower(id);
  if (basic_events_.count(id)) {
    return basic_events_.find(id)->second;

  } else if (gates_.count(id)) {
    return gates_.find(id)->second;

  } else if (house_events_.count(id)) {
    return house_events_.find(id)->second;

  } else {
    std::string msg = "Undefined event: " + reference;
    throw ValidationError(msg);
  }
}

void Model::AddHouseEvent(const HouseEventPtr& house_event) {
  std::string name = house_event->name();
  boost::to_lower(name);

  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Event " + house_event->name() + " already exists.";
    throw ValidationError(msg);
  }
  house_events_.insert(std::make_pair(name, house_event));
}

boost::shared_ptr<HouseEvent> Model::GetHouseEvent(
    const std::string& reference) {
  std::string id = reference;
  boost::to_lower(id);
  if (house_events_.count(id)) {
    return house_events_.find(id)->second;

  } else {
    std::string msg = "Undefined house event: " + reference;
    throw ValidationError(msg);
  }
}

void Model::AddBasicEvent(const BasicEventPtr& basic_event) {
  std::string name = basic_event->name();
  boost::to_lower(name);

  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Event " + basic_event->name() + " already exists.";
    throw ValidationError(msg);
  }
  basic_events_.insert(std::make_pair(name, basic_event));
}

boost::shared_ptr<BasicEvent> Model::GetBasicEvent(
    const std::string& reference) {
  std::string id = reference;
  boost::to_lower(id);
  if (basic_events_.count(id)) {
    return basic_events_.find(id)->second;

  } else {
    std::string msg = "Undefined basic event: " + reference;
    throw ValidationError(msg);
  }
}

void Model::AddGate(const GatePtr& gate) {
  std::string name = gate->name();
  boost::to_lower(name);

  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    std::string msg = "Event " + gate->name() + " already exists.";
    throw ValidationError(msg);
  }
  gates_.insert(std::make_pair(name, gate));
}

boost::shared_ptr<Gate> Model::GetGate(const std::string& reference) {
  std::string id = reference;
  boost::to_lower(id);
  if (gates_.count(id)) {
    return gates_.find(id)->second;

  } else {
    std::string msg = "Undefined gate: " + reference;
    throw ValidationError(msg);
  }
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->name();
  boost::to_lower(name);

  if (ccf_groups_.count(name)) {
    std::string msg = "CCF group " + ccf_group->name() + " already exists.";
    throw ValidationError(msg);
  }
  ccf_groups_.insert(std::make_pair(name, ccf_group));
}

}  // namespace scram
