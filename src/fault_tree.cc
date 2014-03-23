// Implementation of fault tree analysis
#include "risk_analysis.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "error.h"
#include "event.h"

namespace scram {

FaultTree::FaultTree(std::string analysis)
    : analysis_(analysis),
      top_event_id_("") {
  // add valid gates
  gates_.insert("and");
  gates_.insert("or");
  gates_.insert("basic");
}

void FaultTree::process_input(std::string input_file) {
  std::ifstream ifile(input_file.c_str());
  if (!ifile.good()) {
    std::string msg = "Input instruction file is not accessible.";
    throw(scram::IOError(msg));
  }
  input_file_ = input_file;

  std::string line = "";  // case incensitive input
  std::string orig_line = "";  // line with capitazilations preserved
  std::vector<std::string> no_comments;  // to hold lines without comments
  std::vector<std::string> args;  // to hold args after splitting the line
  std::vector<std::string> orig_args;  // original input with capitalizations
  std::string parent = "";
  std::string id = "";
  std::string type = "";
  bool block_started = false;

  for (int nline = 1; getline(ifile, line); ++nline) {
    // remove trailing spaces
    boost::trim(line);
    // check if line is empty
    if (line == "") continue;

    // remove comments starting with # sign
    boost::split(no_comments, line, boost::is_any_of("#"),
                 boost::token_compress_on);
    line = no_comments[0];

    // check if line is comment only
    if (line == "") continue;

    // trim again for spaces before in-line comments
    boost::trim(line);

    // preserve original line for name extraction
    orig_line = line;

    // convert to lower case
    boost::to_lower(line);

    // get args from the line
    boost::split(args, line, boost::is_any_of(" "),
                 boost::token_compress_on);


    switch (args.size()) {
      case 1: {
        if (args[0] == "{") {
          // check if this is a new start of a block
          if (block_started) {
            std::stringstream msg;
            msg << "Line " << nline << " : " << "found second '{' before"
                << " finishing the current block.";
            throw scram::ValidationError(msg.str());
          }
          // new block successfully started
          block_started = true;

        } else if ( args[0] == "}") {
          // check if this is an end of a block
          if (!block_started) {
            std::stringstream msg;
            msg << "Line " << nline << " : " << " found '}' before starting"
                << " a new block.";
            throw scram::ValidationError(msg.str());
          }
          // end of the block detected

          // Add a node with the gathered information
          FaultTree::add_node_(parent, id, type, nline);

          // refresh values
          parent = "";
          id = "";
          type = "";
          block_started = false;

        } else {
          std::stringstream msg;
          msg << "Line " << nline << " : " << "undefined input.";
          throw scram::ValidationError(msg.str());
        }

        continue;  // continue the loop
      }
      case 2: {
        if (args[0] == "parent" && parent == "") {
          parent = args[1];
        } else if (args[0] == "id" && id == "") {
          id = args[1];
          // extract and save original id with capitalizations
          // get args from the original line
          boost::split(orig_args, orig_line, boost::is_any_of(" "),
                       boost::token_compress_on);
          // populate names
          orig_ids_.insert(std::make_pair(id, orig_args[1]));

        } else if (args[0] == "type" && type == "") {
          type = args[1];
          // check if gate/event type is valid
          if (!gates_.count(type)) {
            std::stringstream msg;
            boost::to_upper(type);
            msg << "Line " << nline << " : " << "invalid input arguments."
                << " Do not support this '" << type
                << "' gate/event type.";
            throw scram::ValidationError(msg.str());
          }
        } else {
          // There may go other parameters for FTA
          // For now, just throw an error
          std::stringstream msg;
          msg << "Line " << nline << " : " << "invalid input arguments."
              << " Check spelling and doubly defined parameters inside"
              << " this block.";
          throw scram::ValidationError(msg.str());
        }

        break;
      }
      default: {
        std::stringstream msg;
        msg << "Line " << nline << " : " << "more than 2 arguments.";
        throw scram::ValidationError(msg.str());
      }
    }
  }

  // Defensive check if the top event has at least on child
  if (top_event_->children().size() == 0) {
    std::stringstream msg;
    msg << "The top event does not have intermidiate or basic events.";
    throw scram::ValidationError(msg.str());
  }

  // Check if there is any leaf intermidiate events
  std::string inter_leaf = FaultTree::is_leaf_();
  if (inter_leaf != "") {
    std::stringstream msg;
    msg << "At least one intermidiate event is without basic events."
        << " The id of the intermidiate event is " << inter_leaf;
    throw scram::ValidationError(msg.str());
  }
}

void FaultTree::populate_probabilities(std::string prob_file) {
  // Not yet implemented
}

void FaultTree::graphing_instructions() {
  // Not yet implemented
}

void FaultTree::analyze() {
  // Not yet implemented
}

void FaultTree::add_node_(std::string parent, std::string id, std::string type,
                         int nline) {
  // Initialize events
  if (parent == "none") {
    // check for inaccurate second top event
    if (top_event_id_ == "") {
      top_event_id_ = id;
      top_event_ = new scram::TopEvent(top_event_id_);

      // top event cannot be basic
      if (type == "basic") {
        std::stringstream msg;
        msg << "Line " << nline << " : " << "invalid input arguments."
            << " Top event cannot be basic.";
        throw scram::ValidationError(msg.str());
      }

      top_event_->gate(type);

    } else {
      // Another top event is detected
      std::stringstream msg;
      msg << "Line " << nline << " : " << "invalid input arguments."
          << "The input cannot contain more than one top event.";
      throw scram::ValidationError(msg.str());
    }

  } else if (type == "basic") {
    scram::BasicEvent* b_event = new BasicEvent(id);
    if (parent == top_event_id_){
      b_event->add_parent(top_event_);
      top_event_->add_child(b_event);
      basic_events_.insert(std::make_pair(id, b_event));

    } else if (inter_events_.count(parent)) {
      b_event->add_parent(inter_events_[parent]);
      inter_events_[parent]->add_child(b_event);
      basic_events_.insert(std::make_pair(id, b_event));

    } else {
      // parent is undefined
      std::stringstream msg;
      msg << "Line " << nline << " : " << "invalid input arguments."
          << "Parent of this basic event is not defined.";
      throw scram::ValidationError(msg.str());
    }

  } else {
    // this must be a new intermidiate event
    if (inter_events_.count(id)) {
      // doubly defined intermidiate event
      std::stringstream msg;
      msg << "Line " << nline << " : " << "Intermidiate event is"
          << " doubly defined.";
      throw scram::ValidationError(msg.str());
    }

    scram::InterEvent* i_event = new scram::InterEvent(id);

    if (parent == top_event_id_) {
      i_event->parent(top_event_);
      top_event_->add_child(i_event);
      inter_events_.insert(std::make_pair(id, i_event));

    } else if (inter_events_.count(parent)) {
      i_event->parent(inter_events_[parent]);
      inter_events_[parent]->add_child(i_event);
      inter_events_.insert(std::make_pair(id, i_event));

    } else {
      // parent is undefined
      std::stringstream msg;
      msg << "Line " << nline << " : " << "invalid input arguments."
          << "Parent of this intermidiate event is not defined.";
      throw scram::ValidationError(msg.str());
    }
  }

  // check if a top event defined the first
  if (top_event_id_ == "") {
    std::stringstream msg;
    msg << "Line " << nline << " : " << "invalid input arguments."
        << "Top event must be defined first.";
    throw scram::ValidationError(msg.str());
  }
}

std::string FaultTree::is_leaf_ () {
  std::map<std::string, scram::InterEvent*>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    if (it->second->children().size() == 0) {
      return orig_ids_[it->second->id()];
    }
  }

  return "";
}

}  // namespace scram
