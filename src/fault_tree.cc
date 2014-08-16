/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <iostream>
#include <iterator>
#include <typeinfo>

#include <boost/algorithm/string.hpp>

namespace scram {

FaultTree::FaultTree(std::string name)
    : name_(name),
      top_event_id_(""),
      warnings_("") {}

void FaultTree::AddGate(const GatePtr& gate) {
  if (top_event_id_ == "") {
    top_event_ = gate;
  } else {
    if (inter_events_.count(gate->id()) || gate->id() == top_event_id_) {
      std::stringstream msg;
      msg << "Trying to doubly define a gate";
      throw scram::ValueError(msg.str());
    }
    inter_events_.insert(std::make_pair(gate->id(), gate));

    // Erase this gate from the leaf container.
    leafs_.erase(gate->id());
  }

  // Update the leaf container.
  const std::map<std::string, EventPtr>* children = &gate->children();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    leafs_.insert(it->first);
  }
}

}  // namespace scram
