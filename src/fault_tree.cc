/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <map>

#include <boost/pointer_cast.hpp>

#include "ccf_group.h"
#include "cycle.h"
#include "error.h"

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

void FaultTree::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  if (ccf_groups_.count(ccf_group->name())) {
    throw ValidationError("Trying to re-add ccf group " +
                          ccf_group->name() + ".");
  }
  ccf_groups_.insert(std::make_pair(ccf_group->name(), ccf_group));
}

void FaultTree::Validate() {
  // Detects top events.
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    FaultTree::MarkNonTopGates(it->second);
  }
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    if (it->second->mark() != "non-top") top_events_.push_back(it->second);
  }
}

void FaultTree::MarkNonTopGates(const GatePtr& gate) {
  typedef boost::shared_ptr<Event> EventPtr;
  if (gate->mark() == "non-top") return;
  FaultTree::MarkNonTopGates(gate->formula());
}

void FaultTree::MarkNonTopGates(const FormulaPtr& formula) {
  typedef boost::shared_ptr<Event> EventPtr;
  std::map<std::string, EventPtr>::const_iterator it;
  const std::map<std::string, EventPtr>* children = &formula->event_args();
  for (it = children->begin(); it != children->end(); ++it) {
    GatePtr child_gate = boost::dynamic_pointer_cast<Gate>(it->second);
    if (child_gate && child_gate->container() == name_) {
      FaultTree::MarkNonTopGates(child_gate);
      child_gate->mark("non-top");
    }
  }
  const std::set<FormulaPtr>* formula_args = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formula_args->begin(); it_f != formula_args->end(); ++it_f) {
    FaultTree::MarkNonTopGates(*it_f);
  }
}

}  // namespace scram
