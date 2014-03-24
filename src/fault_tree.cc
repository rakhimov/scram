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
#include <boost/lexical_cast.hpp>

#include "error.h"
#include "event.h"

namespace scram {

FaultTree::FaultTree(std::string analysis)
    : analysis_(analysis),
      top_event_id_(""),
      input_file_("") {
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

        } else if (args[0] == "}") {
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

  // Defensive check if the top event has at least one child
  if (top_event_->children().size() == 0) {
    std::stringstream msg;
    msg << "The top event does not have intermidiate or basic events.";
    throw scram::ValidationError(msg.str());
  }

  // Check if there is any leaf intermidiate events
  std::string leaf_inters = FaultTree::inters_no_child_();
  if (leaf_inters != "") {
    std::stringstream msg;
    msg << "Missing children for follwing intermidiate events:\n";
    msg << leaf_inters;
    throw scram::ValidationError(msg.str());
  }
}

void FaultTree::populate_probabilities(std::string prob_file) {
  // Check if input file with tree instructions has already been read
  if (input_file_ == "") {
    std::string msg = "Read input file with tree instructions before attaching"
                      " probabilities.";
    throw scram::IOError(msg);
  }

  std::ifstream pfile(prob_file.c_str());
  if (!pfile.good()) {
    std::string msg = "Probability file is not accessible.";
    throw(scram::IOError(msg));
  }

  std::string line = "";  // case incensitive input
  std::vector<std::string> no_comments;  // to hold lines without comments
  std::vector<std::string> args;  // to hold args after splitting the line

  std::string id = "";  // name id of a basic event
  double p = -1;  // probability of a basic event
  bool block_started = false;

  for (int nline = 1; getline(pfile, line); ++nline) {
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

        } else if (args[0] == "}") {
          // check if this is an end of a block
          if (!block_started) {
            std::stringstream msg;
            msg << "Line " << nline << " : " << " found '}' before starting"
                << " a new block.";
            throw scram::ValidationError(msg.str());
          }
          // end of the block detected
          // refresh values
          id = "";
          p = -1;
          block_started = false;

        } else {
          std::stringstream msg;
          msg << "Line " << nline << " : " << "undefined input.";
          throw scram::ValidationError(msg.str());
        }

        continue;  // continue the loop
      }
      case 2: {
        id = args[0];
        try {
          p = boost::lexical_cast<double>(args[1]);
        } catch (boost::bad_lexical_cast err) {
          std::stringstream msg;
          msg << "Line " << nline << " : " << "incorrect probability input.";
          throw scram::ValidationError(msg.str());
        }

        try {
          // Add probability of a basic event
          FaultTree::add_prob_(id, p);
        } catch (scram::ValidationError& err_1) {
          std::stringstream new_msg;
          new_msg << "Line " << nline << " : " << err_1.msg();
          throw scram::ValidationError(new_msg.str());
        } catch (scram::ValueError& err_2) {
          std::stringstream new_msg;
          new_msg << "Line " << nline << " : " << err_2.msg();
          throw scram::ValueError(new_msg.str());
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

  // Check if all basic events have probabilities initialized
  std::string uninit_basics = FaultTree::basics_no_prob_();
  if (uninit_basics != "") {
    std::stringstream msg;
    msg << "Missing probabilities for following basic events:\n";
    msg << uninit_basics;
    throw scram::ValidationError(msg.str());
  }

}

void FaultTree::graphing_instructions() {
  // Not yet implemented
}

void FaultTree::analyze() {
  // Generate minimal cut-sets: Naive method
  // Rule 1. Each OR gate generates new rows in the table of cut sets
  // Rule 2. Each AND gate generates new columns in the table of cut sets
  // After each of the above steps:
  // Rule 3. Eliminate redundant elements that appear multiple times in a cut
  // set
  // Rule 4. Eliminate non-minimal cut sets
  //
  // Implementation:
  // Level i : get events into

  // container for cut sets with intermidiate events
  std::vector< std::set<std::string> > inter_sets;

  // container for cut sets with basic events only
  std::vector< std::set<std::string> > cut_sets;

  // container for minimal cut sets
  std::vector< std::set<std::string> > minimal_cut_sets;

  // Level 0: Start with the top event
  std::set<std::string> top;
  top.insert(top_event_id_);
  // populate intermidiate and basic events of the top
  std::map<std::string, scram::Event*> tops_children = top_event_->children();
  // iterator for children of top and intermidiate events
  std::map<std::string, scram::Event*>::iterator it_child;
  // this is logic for OR gate
  if (top_event_->gate() == "or") {
    for (it_child = tops_children.begin(); it_child != tops_children.end();
         ++it_child) {
      std::set<std::string> tmp_set;
      tmp_set.insert(it_child->first);
      inter_sets.push_back(tmp_set);
    }
  }

  // an iterator for a set
  std::set<std::string>::iterator it_set;

  // Generate cut sets
  while (!inter_sets.empty()) {
    // get rightmost set
    std::set<std::string> tmp_set = inter_sets.back();
    // delete rightmost set
    inter_sets.pop_back();
    // iterate till finding intermidiate event




  }
  // Compute probabilities
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

void FaultTree::add_prob_(std::string id, double p) {
  // Check if the basic event is in this tree
  if (basic_events_.count(id) == 0) {
    std::string msg = "Basic event " + id + " was not iniated in this tree.";
    throw scram::ValidationError(msg);
  }

  basic_events_[id]->p(p);

}

std::string FaultTree::inters_no_child_ () {
  std::string leaf_inters = "";
  std::map<std::string, scram::InterEvent*>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    try {
      it->second->children();
    } catch (scram::ValueError& err) {
      leaf_inters += it->first;
      leaf_inters += "\n";
    }
  }

  return leaf_inters;
}

std::string FaultTree::basics_no_prob_() {
  std::string uninit_basics = "";
  std::map<std::string, scram::BasicEvent*>::iterator it;
  for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
    try {
      it->second->p();
    } catch (scram::ValueError& err) {
      uninit_basics += it->first;
      uninit_basics += "\n";
    }
  }

  return uninit_basics;
}

}  // namespace scram
