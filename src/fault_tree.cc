/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <boost/pointer_cast.hpp>

#include <iterator>

namespace scram {

FaultTree::FaultTree(std::string name)
    : name_(name),
      top_event_id_(""),
      changed_(true),
      warnings_("") {}

void FaultTree::AddGate(GatePtr& gate) {
  if (!changed_) changed_ = true;

  if (top_event_id_ == "") {
    top_event_ = gate;
    top_event_id_ = gate->id();
  } else {
    if (inter_events_.count(gate->id()) || gate->id() == top_event_id_) {
      std::string msg =  "Trying to doubly define a gate";
      throw scram::ValueError(msg);
    }
    // @todo Check if this gate has a valid parent in this tree.
    inter_events_.insert(std::make_pair(gate->id(), gate));
  }
}

void FaultTree::GatherPrimaryEvents() {
  changed_ = false;  // Assumes that the tree is fully developed.

  FaultTree::GetPrimaryEvents(top_event_);

  boost::unordered_map<std::string, GatePtr>::iterator git;
  for (git = inter_events_.begin(); git != inter_events_.end(); ++git) {
    FaultTree::GetPrimaryEvents(git->second);
  }
}

void FaultTree::GetPrimaryEvents(GatePtr& gate) {
  const std::map<std::string, EventPtr>* children = &gate->children();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    if (!inter_events_.count(it->first)) {
      PrimaryEventPtr primary_event =
          boost::dynamic_pointer_cast<scram::PrimaryEvent>(it->second);
      assert(primary_event != 0);  // The tree must be fully defined.
                                   // @todo May change to an exception that
                                   // indicates un-initialized event.
      primary_events_.insert(std::make_pair(it->first, primary_event));
    }
  }
}

}  // namespace scram
