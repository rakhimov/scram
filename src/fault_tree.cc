/// @file fault_tree.cc
/// Implementation of fault tree and component containers.
#include "fault_tree.h"

#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

#include "ccf_group.h"
#include "cycle.h"
#include "error.h"
#include "expression.h"

namespace scram {

Component::Component(const std::string& name, const std::string& base_path,
                     bool is_public)
    : name_(name),
      Role::Role(is_public, base_path) {}

void Component::AddGate(const GatePtr& gate) {
  std::string name = gate->name();
  boost::to_lower(name);
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    throw ValidationError("Duplicate event " + gate->name());
  }
  gates_.insert(std::make_pair(name, gate));
}

void Component::AddBasicEvent(const BasicEventPtr& basic_event) {
  std::string name = basic_event->name();
  boost::to_lower(name);
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    throw ValidationError("Duplicate event " + basic_event->name());
  }
  basic_events_.insert(std::make_pair(name, basic_event));
}

void Component::AddHouseEvent(const HouseEventPtr& house_event) {
  std::string name = house_event->name();
  boost::to_lower(name);
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    throw ValidationError("Duplicate event " + house_event->name());
  }
  house_events_.insert(std::make_pair(name, house_event));
}

void Component::AddParameter(const ParameterPtr& parameter) {
  std::string name = parameter->name();
  boost::to_lower(name);
  if (parameters_.count(name)) {
    throw ValidationError("Duplicate parameter " + parameter->name());
  }
  parameters_.insert(std::make_pair(name, parameter));
}

void Component::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->name();
  boost::to_lower(name);
  if (ccf_groups_.count(name)) {
    throw ValidationError("Duplicate CCF group " + ccf_group->name());
  }
  ccf_groups_.insert(std::make_pair(name, ccf_group));
  std::map<std::string, BasicEventPtr>::const_iterator it;
  for (it = ccf_group->members().begin(); it != ccf_group->members().end();
       ++it) {
    if (gates_.count(it->first) || basic_events_.count(it->first) ||
        house_events_.count(it->first)) {
      throw ValidationError("Duplicate event " + it->second->name() +
                            " from CCF group " + ccf_group->name());
    }
    basic_events_.insert(*it);
  }
}

void Component::AddComponent(const ComponentPtr& component) {
  std::string name = component->name();
  boost::to_lower(name);
  if (components_.count(name)) {
    throw ValidationError("Duplicate component " + component->name());
  }
  components_.insert(std::make_pair(name, component));
}

void Component::GatherGates(boost::unordered_set<GatePtr>* gates) {
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    gates->insert(it->second);
  }
  boost::unordered_map<std::string, ComponentPtr>::iterator it_comp;
  for (it_comp = components_.begin(); it_comp != components_.end(); ++it_comp) {
    it_comp->second->GatherGates(gates);
  }
}

FaultTree::FaultTree(const std::string& name) : Component::Component(name) {}

void FaultTree::Validate() {
  boost::unordered_set<GatePtr> gates;
  Component::GatherGates(&gates);
  // Detects top events.
  boost::unordered_set<GatePtr>::iterator it;
  for (it = gates.begin(); it != gates.end(); ++it) {
    FaultTree::MarkNonTopGates(*it, gates);
  }
  for (it = gates.begin(); it != gates.end(); ++it) {
    if ((*it)->mark() != "non-top") top_events_.push_back(*it);
    (*it)->mark("");
  }
}

void FaultTree::MarkNonTopGates(const GatePtr& gate,
                                const boost::unordered_set<GatePtr>& gates) {
  if (gate->mark() == "non-top") return;
  FaultTree::MarkNonTopGates(gate->formula(), gates);
}

void FaultTree::MarkNonTopGates(const FormulaPtr& formula,
                                const boost::unordered_set<GatePtr>& gates) {
  typedef boost::shared_ptr<Event> EventPtr;
  std::map<std::string, EventPtr>::const_iterator it;
  const std::map<std::string, EventPtr>* children = &formula->event_args();
  for (it = children->begin(); it != children->end(); ++it) {
    GatePtr child_gate = boost::dynamic_pointer_cast<Gate>(it->second);
    if (child_gate && gates.count(child_gate)) {
      FaultTree::MarkNonTopGates(child_gate, gates);
      child_gate->mark("non-top");
    }
  }
  const std::set<FormulaPtr>* formula_args = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formula_args->begin(); it_f != formula_args->end(); ++it_f) {
    FaultTree::MarkNonTopGates(*it_f, gates);
  }
}

}  // namespace scram
