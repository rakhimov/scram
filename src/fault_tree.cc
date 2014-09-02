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
      throw scram::ValidationError("Trying to doubly define a gate");
    }
    // Check if this gate has a valid parent in this tree.
    const std::map<std::string, GatePtr>* parents;
    try {
      parents = &gate->parents();
    } catch (ValueError& err) {
      // No parents here.
      throw ValidationError("Gate '" + gate->id() + "' is a dangling gate in" +
                            " a malformed tree input structure. " + err.msg());
    }
    std::map<std::string, GatePtr>::const_iterator it;
    bool parent_found = false;
    for (it = parents->begin(); it != parents->end(); ++it) {
      if (inter_events_.count(it->first) || top_event_id_ == it->first) {
        parent_found = true;
        break;
      }
    }
    if (!parent_found) {
      throw ValidationError("Gate '" + gate->id() + "' has no pre-decleared" +
                            " parent gate in '" + name_ +
                            "' fault tree. This gate is a dangling gate." +
                            " The tree structure input might be malformed.");
    }
    inter_events_.insert(std::make_pair(gate->id(), gate));
  }
}

void FaultTree::Validate() {
  // The gate structure must be checked first.
  std::vector<std::string> path;
  std::set<std::string> visited;
  FaultTree::CheckCyclicity(top_event_, path, visited);

  // Assumes that the tree is fully developed.
  primary_events_.clear();
  // Gather all primary events belonging to this tree.
  FaultTree::GatherPrimaryEvents();
}

void FaultTree::CheckCyclicity(const GatePtr& parent,
                               std::vector<std::string> path,
                               std::set<std::string> visited) {
  path.push_back(parent->id());
  if (visited.count(parent->id())) {
    std::string msg = "Detected a cyclicity in '" + name_ + "' fault tree:\n";
    std::vector<std::string>::iterator it;
    bool path_start = false;
    for (it = path.begin(); it != path.end(); ++it) {
      if (!path_start && *it == parent->id()) {
        path_start = true;
        msg += parent->id();
        continue;
      }
      if (path_start) {
        msg += "->" + *it;
      }
    }
    throw ValidationError(msg);
  } else {
    visited.insert(parent->id());
  }

  const std::map<std::string, EventPtr>* children = &parent->children();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    GatePtr child_gate = boost::dynamic_pointer_cast<scram::Gate>(it->second);
    if (child_gate) {
      if (!inter_events_.count(child_gate->id())) {
        implicit_gates_.insert(std::make_pair(child_gate->id(), child_gate));
        inter_events_.insert(std::make_pair(child_gate->id(), child_gate));
      }
      CheckCyclicity(child_gate, path, visited);
    }
  }
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
