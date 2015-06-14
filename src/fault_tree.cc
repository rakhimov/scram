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
  // Detects the top event. Currently only one top event is allowed.
  /// @todo Add support for multiple top events.
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    if (it->second->IsOrphan()) {
      if (top_event_) {
        throw ValidationError("Multiple top events are detected: " +
                              top_event_->name() + " and " +
                              it->second->name() + " in " + name_ +
                              " fault tree.");
      }
      top_event_ = it->second;
    }
  }
}

}  // namespace scram
