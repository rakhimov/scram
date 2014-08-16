/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace scram {

FaultTree::FaultTree(std::string name)
    : name_(name),
      is_main_(true),
      top_event_id_(""),
      top_detected_(false),
      input_file_(""),
      current_file_(""),
      warnings_("") {}

void FaultTree::AddNode(std::string parent, std::string id,
                        std::string type, int vote_number) {
  // Check if this is a transfer.
  if (type == "transferin") {
    if (parent == "none") {
      top_detected_ = true;
    }
    // Register to call later.
    transfers_.push(std::make_pair(parent, id));  // Parent names are unique.
    trans_calls_.insert(std::make_pair(id, 0));  // Discard if exists already.
    transfer_map_.insert(std::make_pair(parent, id));
    // Check if this is a cyclic inclusion.
    // This line does many things that are tricky and c++ map specific.
    // Find vectors ending with the currently opened file name and append
    // the sub-tree requested to be called later.
    // Attach that subtree if it does not clash with the initiating
    // grandparents.

    trans_tree_[current_file_].push_back(id);

    std::map< std::string, std::vector<std::string> >::iterator it_t;
    for (it_t = trans_tree_.begin(); it_t != trans_tree_.end(); ++it_t) {
      if (!it_t->second.empty() && it_t->second.back() == current_file_) {
        if (it_t->first == id) {
          std::vector<std::string> chain = it_t->second;
          std::stringstream msg;
          msg << "Detected circular inclusion of ( " << id;

          std::vector<std::string>::iterator it;
          for (it = chain.begin(); it != chain.end(); ++it) {
            msg << "->" << *it;
          }
          msg << "->" << id << " )";
          throw scram::ValidationError(msg.str());

        } else {
          it_t->second.push_back(id);
        }
      }
    }
    return;
  }

  // Initialize events.
  if (parent == "none") {
    if (top_detected_ && is_main_) {
      std::stringstream msg;
      msg << "Found a second top event in file where top event is described "
          << "by a transfer sub-tree.";
      throw scram::ValidationError(msg.str());
    }

    if (top_event_id_ == "") {
      top_event_id_ = id;
      top_event_ = GatePtr(new scram::Gate(top_event_id_));

      // Top event cannot be primary.
      if (!gates_.count(type)) {
        std::stringstream msg;
        msg << "Invalid input arguments. " << orig_ids_[top_event_id_]
            << " top event type is not defined correctly. It must be a gate.";
        throw scram::ValidationError(msg.str());
      }

      top_event_->type(type);
      if (type == "vote") top_event_->vote_number(vote_number);

    } else {
      // Another top event is detected.
      std::stringstream msg;
      msg << "Invalid input arguments. The input cannot contain more than"
          << " one top event. " << orig_ids_[id] << " is redundant.";
      throw scram::ValidationError(msg.str());
    }

  } else if (types_.count(type)) {
    // This must be a primary event.
    // Check if this is a re-initialization with a different type.
    if (primary_events_.count(id) && primary_events_[id]->type() != type) {
      std::stringstream msg;
      msg << "Redefining a primary event " << orig_ids_[id]
          << " with a different type.";
      throw scram::ValidationError(msg.str());
    }
    // Detect name clashes.
    if (id == top_event_id_ || inter_events_.count(id)) {
      std::stringstream msg;
      msg << "The id " << orig_ids_[id]
          << " is already assigned to the top or intermediate event.";
      throw scram::ValidationError(msg.str());
    }

    PrimaryEventPtr p_event(new PrimaryEvent(id, type));
    if (parent == top_event_id_) {
      p_event->AddParent(top_event_);
      top_event_->AddChild(p_event);
      primary_events_.insert(std::make_pair(id, p_event));
      // Check for conditional event.
      if (type == "conditional" && top_event_->type() != "inhibit") {
        std::stringstream msg;
        msg << "Parent of " << orig_ids_[id] << " conditional event is not "
            << "an inhibit gate.";
        throw scram::ValidationError(msg.str());
      }
    } else if (inter_events_.count(parent)) {
      p_event->AddParent(inter_events_[parent]);
      inter_events_[parent]->AddChild(p_event);
      primary_events_.insert(std::make_pair(id, p_event));
      // Check for conditional event.
      if (type == "conditional" && inter_events_[parent]->type() != "inhibit") {
        std::stringstream msg;
        msg << "Parent of " << orig_ids_[id] << " conditional event is not "
            << "an inhibit gate.";
        throw scram::ValidationError(msg.str());
      }
    } else {
      // Parent is undefined.
      std::stringstream msg;
      msg << "Invalid input arguments. Parent of " << orig_ids_[id]
          << " primary event is not defined.";
      throw scram::ValidationError(msg.str());
    }
  } else {
    // This must be a new intermediate event.
    if (inter_events_.count(id)) {
      // Doubly defined intermediate event.
      std::stringstream msg;
      msg << orig_ids_[id] << " intermediate event is doubly defined.";
      throw scram::ValidationError(msg.str());
    }
    // Detect name clashes.
    if (id == top_event_id_ || primary_events_.count(id)) {
      std::stringstream msg;
      msg << "The id " << orig_ids_[id]
          << " is already assigned to the top or primary event.";
      throw scram::ValidationError(msg.str());
    }

    GatePtr i_event(new scram::Gate(id));

    if (parent == top_event_id_) {
      i_event->AddParent(top_event_);
      top_event_->AddChild(i_event);
      inter_events_.insert(std::make_pair(id, i_event));

    } else if (inter_events_.count(parent)) {
      i_event->AddParent(inter_events_[parent]);
      inter_events_[parent]->AddChild(i_event);
      inter_events_.insert(std::make_pair(id, i_event));

    } else {
      // Parent is undefined.
      std::stringstream msg;
      msg << "Invalid input arguments. Parent of " << orig_ids_[id]
          << " intermediate event is not defined.";
      throw scram::ValidationError(msg.str());
    }

    i_event -> type(type);
    if (type == "vote") i_event->vote_number(vote_number);
  }
}

void FaultTree::AddProb(std::string id, double p) {
  // Check if the primary event is in this tree.
  if (!primary_events_.count(id)) {
    return;  // Ignore non-existent assignment.
  }

  primary_events_[id]->p(p);
}

void FaultTree::AddProb(std::string id, double p, double time) {
  // Check if the primary event is in this tree.
  if (!primary_events_.count(id)) {
    return;  // Ignore non-existent assignment.
  }

  primary_events_[id]->p(p, time);
}

std::string FaultTree::CheckAllGates() {
  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.

  // Check the top event.
  msg << FaultTree::CheckGate_(top_event_);

  // Check the intermediate events.
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    msg << FaultTree::CheckGate_(it->second);
  }

  return msg.str();
}

std::string FaultTree::CheckGate_(const GatePtr& event) {
  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.
  try {
    std::string gate = event->type();
    // This line throws an error if there are no children.
    int size = event->children().size();
    // Add transfer gates if needed for graphing.
    size += transfer_map_.count(event->id());

    // Gate dependent logic.
    if (gate == "and" || gate == "or" || gate == "nor" || gate == "nand") {
      if (size < 2) {
        boost::to_upper(gate);
        msg << orig_ids_[event->id()] << " : " << gate
            << " gate must have 2 or more "
            << "children.\n";
      }
    } else if (gate == "xor") {
      if (size != 2) {
        boost::to_upper(gate);
        msg << orig_ids_[event->id()] << " : " << gate
            << " gate must have exactly 2 children.\n";
      }
    } else if (gate == "inhibit") {
      if (size != 2) {
        boost::to_upper(gate);
        msg << orig_ids_[event->id()] << " : " << gate
            << " gate must have exactly 2 children.\n";
      } else {
        bool conditional_found = false;
        std::map<std::string, NodePtr> children = event->children();
        std::map<std::string, NodePtr>::iterator it;
        for (it = children.begin(); it != children.end(); ++it) {
          if (primary_events_.count(it->first)) {
            std::string type = primary_events_[it->first]->type();
            if (type == "conditional") {
              if (!conditional_found) {
                conditional_found = true;
              } else {
                boost::to_upper(gate);
                msg << orig_ids_[event->id()] << " : " << gate
                    << " gate must have exactly one conditional event.\n";
              }
            }
          }
        }
        if (!conditional_found) {
          boost::to_upper(gate);
          msg << orig_ids_[event->id()] << " : " << gate
              << " gate is missing a conditional event.\n";
        }
      }
    } else if (gate == "not" || gate == "null") {
      if (size != 1) {
        boost::to_upper(gate);
        msg << orig_ids_[event->id()] << " : " << gate
            << " gate must have exactly one child.";
      }
    } else if (gate == "vote") {
      if (size <= event->vote_number()) {
        boost::to_upper(gate);
        msg << orig_ids_[event->id()] << " : " << gate
            << " gate must have more children that its vote number "
            << event->vote_number() << ".";
      }
    } else {
      boost::to_upper(gate);
      msg << orig_ids_[event->id()] << " : Gate Check failure. No check for "
          << gate << " gate.";
    }
  } catch (scram::ValueError& err) {
    msg << orig_ids_[event->id()] << " : No children detected.";
  }

  return msg.str();
}

std::string FaultTree::PrimariesNoProb_() {
  std::string uninit_primaries = "";
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator it;
  for (it = primary_events_.begin(); it != primary_events_.end(); ++it) {
    try {
      it->second->p();
    } catch (scram::ValueError& err) {
      uninit_primaries += orig_ids_[it->first];
      uninit_primaries += "\n";
    }
  }

  return uninit_primaries;
}

}  // namespace scram
