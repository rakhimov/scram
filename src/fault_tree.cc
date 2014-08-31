/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <boost/pointer_cast.hpp>

#include <iterator>

namespace scram {

FaultTree::FaultTree(std::string name) : name_(name), top_event_id_("") {}

void FaultTree::AddGate(const GatePtr& gate) {
  if (top_event_id_ == "") {
    top_event_ = gate;
    top_event_id_ = gate->id();
  } else {
    if (inter_events_.count(gate->id()) || gate->id() == top_event_id_) {
      std::string msg =  "Trying to doubly define a gate";
      throw scram::ValueError(msg);
    }
    /// @todo Check if this gate has a valid parent in this tree.
    inter_events_.insert(std::make_pair(gate->id(), gate));
  }
}

void FaultTree::Validate() {
  // The gate structure must be checked first.
  // Assumes that the tree is fully developed.
  primary_events_.clear();
  GatherPrimaryEvents();
}

void FaultTree::GatherPrimaryEvents() {
  FaultTree::GetPrimaryEvents(top_event_);

  boost::unordered_map<std::string, GatePtr>::iterator git;
  for (git = inter_events_.begin(); git != inter_events_.end(); ++git) {
    FaultTree::GetPrimaryEvents(git->second);
  }
}

void FaultTree::GetPrimaryEvents(const GatePtr& gate) {
  const std::map<std::string, EventPtr>* children = &gate->children();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    if (!inter_events_.count(it->first)) {
      PrimaryEventPtr primary_event =
          boost::dynamic_pointer_cast<scram::PrimaryEvent>(it->second);

      if (primary_event == 0) {  // The tree must be fully defined.
        throw ValidationError("Node with id '" + it->first +
                              "' was not defined in '" + name_+ "' tree");
      }

      primary_events_.insert(std::make_pair(it->first, primary_event));
    }
  }
}

}  // namespace scram
