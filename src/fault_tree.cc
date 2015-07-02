/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

#include "ccf_group.h"
#include "cycle.h"
#include "error.h"
#include "expression.h"

namespace scram {

FaultTree::FaultTree(std::string name) : name_(name) {}

void FaultTree::AddGate(const GatePtr& gate) {
  if (gates_.count(gate->id())) {
    throw ValidationError("Trying to re-add gate " + gate->name() + ".");
  }
  gates_.insert(std::make_pair(gate->id(), gate));
}

void FaultTree::AddBasicEvent(const BasicEventPtr& basic_event) {
  if (basic_events_.count(basic_event->id())) {
    throw ValidationError("Trying to re-add basic event " +
                          basic_event->name() + ".");
  }
  basic_events_.insert(std::make_pair(basic_event->id(), basic_event));
}

void FaultTree::AddHouseEvent(const HouseEventPtr& house_event) {
  if (house_events_.count(house_event->id())) {
    throw ValidationError("Trying to re-add house event " +
                          house_event->name() + ".");
  }
  house_events_.insert(std::make_pair(house_event->id(), house_event));
}

void FaultTree::AddParameter(const ParameterPtr& parameter) {
  std::string name = parameter->name();
  boost::to_lower(name);
  if (parameters_.count(name)) {
    throw ValidationError("Trying to re-add parameter " +
                          parameter->name() + ".");
  }
  parameters_.insert(std::make_pair(name, parameter));
}

void FaultTree::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->id();
  boost::to_lower(name);
  if (ccf_groups_.count(name)) {
    throw ValidationError("Trying to re-add ccf group " +
                          ccf_group->name() + ".");
  }
  ccf_groups_.insert(std::make_pair(name, ccf_group));
}

void FaultTree::AddComponent(const ComponentPtr& component) {
  std::string name = component->name();
  boost::to_lower(name);
  if (components_.count(name)) {
    throw ValidationError("Component " +
                          component->name() + " already exists at this level.");
  }
  components_.insert(std::make_pair(name, component));
}

void FaultTree::Validate() {
  boost::unordered_set<GatePtr> gates;
  FaultTree::GatherGates(&gates);
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

void FaultTree::GatherGates(boost::unordered_set<GatePtr>* gates) {
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    gates->insert(it->second);
  }
  boost::unordered_map<std::string, ComponentPtr>::iterator it_comp;
  for (it_comp = components_.begin(); it_comp != components_.end(); ++it_comp) {
    it_comp->second->GatherGates(gates);
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

Component::Component(const std::string& name, const std::string& base_path,
                     bool is_public)
    : FaultTree::FaultTree(name),
      Role::Role(is_public, base_path) {}

}  // namespace scram
