/// @file risk_analysis.cc
#include "risk_analysis.h"

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

namespace scram {

RiskAnalysis::RiskAnalysis(std::string config_file)
    : prob_requested_(false),
      parent_(""),
      id_(""),
      type_(""),
      vote_number_(-1) {
      // Add valid gate types.
      gate_types_.insert("and");
      gate_types_.insert("or");
      gate_types_.insert("not");
      gate_types_.insert("nor");
      gate_types_.insert("nand");
      gate_types_.insert("xor");
      gate_types_.insert("null");
      gate_types_.insert("inhibit");
      gate_types_.insert("vote");

      // Add valid primary event types.
      types_.insert("basic");
      types_.insert("undeveloped");
      types_.insert("house");
      types_.insert("conditional");

      // Initialize a fault tree with a default name.
      FaultTreePtr fault_tree_;

      fta_ = new FaultTreeAnalysis("default");
    }

void RiskAnalysis::ProcessInput(std::string input_file) {
  std::ifstream ifile(input_file.c_str());
  if (!ifile.good()) {
    std::string msg = input_file +  " : Tree file is not accessible.";
    throw(scram::IOError(msg));
  }
  input_file_ = input_file;

  std::string line = "";  // Case insensitive input.
  std::string orig_line = "";  // Line with capitalizations preserved.
  std::vector<std::string> args;  // To hold args after splitting the line.

  // Error messages.
  std::stringstream msg;
  msg << "In " << input_file << ", ";

  for (int nline = 1; getline(ifile, line); ++nline) {
    if (!RiskAnalysis::GetArgs(line, orig_line, args)) continue;

    RiskAnalysis::InterpretArgs(nline, msg, args, orig_line);
  }

  // Check if all gates have a right number of children.
  std::string bad_gates = RiskAnalysis::CheckAllGates();
  if (bad_gates != "") {
    std::stringstream msg;
    msg << "\nThere are problems with the following gates:\n";
    msg << bad_gates;
    throw scram::ValidationError(msg.str());
  }
}

void RiskAnalysis::PopulateProbabilities(std::string prob_file) {
  // Check if input file with tree instructions has already been read.
  if (input_file_ == "") {
    std::string msg = "Read input file with tree instructions before attaching"
        " probabilities.";
    throw scram::Error(msg);
  }

  std::ifstream pfile(prob_file.c_str());
  if (!pfile.good()) {
    std::string msg = prob_file + " : Probability file is not accessible.";
    throw scram::IOError(msg);
  }

  prob_requested_ = true;  // Switch indicator.

  std::string line = "";  // Case insensitive input.
  std::string orig_line = "";  // Original line.
  std::vector<std::string> args;  // To hold args after splitting the line.

  std::string id = "";  // Name id of a primary event.
  double p = -1;  // Probability of a primary event.
  double time = -1;  // Time for lambda type probability.
  bool block_started = false;
  std::string block_type = "p-model";  // Default probability type.
  bool block_set = false;  // If the block type defined by a user explicitly.

  // Error messages.
  std::stringstream msg;
  msg << "In " << prob_file << ", ";

  for (int nline = 1; getline(pfile, line); ++nline) {
    if (!RiskAnalysis::GetArgs(line, orig_line, args)) continue;

    switch (args.size()) {
      case 1: {
        if (args[0] == "{") {
        } else if (args[0] == "}") {

          // Refresh values.
          id = "";
          p = -1;
          time = -1;
          block_started = false;
          block_type = "p-model";
          block_set = false;
        }
        break;
      }
      case 2: {

        id = args[0];

        if (id == "block") {
          block_type = args[1];
          block_set = true;
          break;
        }

        if (id == "time" && block_type == "l-model") {
          if (time == -1) {
            try {
              time = boost::lexical_cast<double>(args[1]);
            } catch (boost::bad_lexical_cast& err) {
              msg << "Line " << nline << " : " << "Incorrect time input.";
              throw scram::ValidationError(msg.str());
            }
            if (time < 0) {
              msg << "Line " << nline << " : " << "Invalid value for time.";
              throw scram::ValidationError(msg.str());
            }
          } else {
            msg << "Line " << nline << " : " << "Doubly defining time.";
            throw scram::ValidationError(msg.str());
          }
        }

        try {
          p = boost::lexical_cast<double>(args[1]);
        } catch (boost::bad_lexical_cast& err) {
          msg << "Line " << nline << " : " << "Incorrect probability input.";
          throw scram::ValidationError(msg.str());
        }

        try {
          // Add probability of a primary event.
          if (block_type == "p-model") {
            RiskAnalysis::AddProb(id, p);
          } else if (block_type == "l-model") {
            if (time == -1) {
              msg << "Line " << nline << " : " << "L-Model Time is not given";
              throw scram::ValidationError(msg.str());
            }
            RiskAnalysis::AddProb(id, p, time);
          }
        } catch (scram::ValueError& err_2) {
          msg << "Line " << nline << " : " << err_2.msg();
          throw scram::ValueError(msg.str());
        }
        break;
      }
      default: {
        msg << "Line " << nline << " : " << "More than 2 arguments.";
        throw scram::ValidationError(msg.str());
      }
    }
  }

  // Check if all primary events have probabilities initialized.
  std::string uninit_primaries = RiskAnalysis::PrimariesNoProb();
  if (uninit_primaries != "") {
    msg << "Missing probabilities for following basic events:\n";
    msg << uninit_primaries;
    throw scram::ValidationError(msg.str());
  }
}

void RiskAnalysis::GraphingInstructions() {
  /// @todo Make this exception safe with a smart pointer.
  Grapher* gr = new Grapher();
  gr->GraphFaultTree(fault_tree_, orig_ids_, prob_requested_, input_file_);
  delete gr;
}

void RiskAnalysis::Analyze() {
  fta_->Analyze(fault_tree_, orig_ids_, prob_requested_);
}

void RiskAnalysis::Report(std::string output) {
  /// @todo Make this exception safe with a smart pointer.
  Reporter* rp = new Reporter();
  rp->ReportFta(fta_, output);
  delete rp;
}

bool RiskAnalysis::GetArgs(std::string& line, std::string& orig_line,
                            std::vector<std::string>& args) {
  std::vector<std::string> no_comments;  // To hold lines without comments.

  // Remove trailing spaces.
  boost::trim(line);
  // Check if line is empty.
  if (line == "") return false;

  // Remove comments starting with # sign.
  boost::split(no_comments, line, boost::is_any_of("#"),
               boost::token_compress_on);
  line = no_comments[0];

  // Check if line is comment only.
  if (line == "") return false;

  // Trim again for spaces before in-line comments.
  boost::trim(line);

  // Preserve original line for name extraction.
  orig_line = line;

  // Convert to lower case.
  boost::to_lower(line);

  // Get args from the line.
  boost::split(args, line, boost::is_any_of(" "),
               boost::token_compress_on);
  return true;
}

void RiskAnalysis::InterpretArgs(int nline, std::stringstream& msg,
                                  std::vector<std::string>& args,
                                  std::string& orig_line) {

  switch (args.size()) {
    case 1: {
      if (args[0] == "{") {

      } else if (args[0] == "}") {

        // Check if all needed arguments for an event are received.
        if (parent_ == "") {
          msg << "Line " << nline << " : " << "Missing parent in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        } else if (id_ == "") {
          msg << "Line " << nline << " : " << "Missing id in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        } else if (type_ == "") {
          msg << "Line " << nline << " : " << "Missing type in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        } else if (type_ == "vote" && vote_number_ == -1) {
          msg << "Line " << nline << " : " << "Missing Vote Number in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        }

        try {
          // Add a node with the gathered information.
          RiskAnalysis::AddNode(parent_, id_, type_, vote_number_);
        } catch (scram::ValidationError& err) {
          msg << "Line " << nline << " : " << err.msg();
          throw scram::ValidationError(msg.str());
        }

        // Refresh values.
        parent_ = "";
        id_ = "";
        type_ = "";
        vote_number_ = -1;

      } else {
        msg << "Line " << nline << " : " << "Undefined input.";
        throw scram::ValidationError(msg.str());
      }

      break;
    }
    case 2: {
      if (args[0] == "parent" && parent_ == "") {
        parent_ = args[1];
      } else if (args[0] == "id" && id_ == "") {
        id_ = args[1];
        // Extract and save original id with capitalizations.
        // Get args from the original line.
        std::vector<std::string> orig_args;
        boost::split(orig_args, orig_line, boost::is_any_of(" "),
                     boost::token_compress_on);
        // Populate names.
        orig_ids_.insert(std::make_pair(id_, orig_args[1]));

      } else if (args[0] == "type" && type_ == "") {
        type_ = args[1];
        // Check if gate/event type is valid.
        if (!gate_types_.count(type_) && !types_.count(type_)) {
          boost::to_upper(type_);
          msg << "Line " << nline << " : " << "Invalid input arguments."
              << " Do not support this '" << type_
              << "' gate/event type.";
          throw scram::ValidationError(msg.str());
        }
      } else if (args[0] == "votenumber" && vote_number_ == -1) {
        try {
          vote_number_ = boost::lexical_cast<int>(args[1]);
        } catch (boost::bad_lexical_cast& err) {
          msg << "Line " << nline << " : " << "Incorrect vote number input.";
          throw scram::ValidationError(msg.str());
        }
      } else {
        // There may go other parameters for FTA.
        // For now, just throw an error.
        msg << "Line " << nline << " : " << "Invalid input arguments."
            << " Check spelling and doubly defined parameters inside"
            << " this block.";
        throw scram::ValidationError(msg.str());
      }

      break;
    }
    default: {
      msg << "Line " << nline << " : " << "More than 2 arguments.";
      throw scram::ValidationError(msg.str());
    }
  }
}

void RiskAnalysis::AddNode(std::string parent,
                            std::string id,
                            std::string type,
                            int vote_number) {
  // Initialize events.
  if (parent == "none") {
    GatePtr top_event(new scram::Gate(id));

    gates_.insert(std::make_pair(id, top_event));
    all_events_.insert(std::make_pair(id, top_event));

    fault_tree_ = FaultTreePtr(new FaultTree(id));
    fault_tree_->AddGate(top_event);

    // Top event cannot be primary.
    if (!gate_types_.count(type)) {
      std::stringstream msg;
      msg << "Invalid input arguments. " << orig_ids_[id]
          << " top event type is not defined correctly. It must be a gate.";
      throw scram::ValidationError(msg.str());
    }

    top_event->type(type);
    if (type == "vote") top_event->vote_number(vote_number);

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
    if (gates_.count(id)) {
      std::stringstream msg;
      msg << "The id " << orig_ids_[id]
          << " is already assigned to a gate.";
      throw scram::ValidationError(msg.str());
    }

    PrimaryEventPtr p_event;
    // Check if this is a re-use of a primary event.
    if (primary_events_.count(id)) {
      p_event = primary_events_[id];
    } else {
      p_event = PrimaryEventPtr(new PrimaryEvent(id, type));
      primary_events_.insert(std::make_pair(id, p_event));
      all_events_.insert(std::make_pair(id, p_event));
    }

    if (gates_.count(parent)) {
      p_event->AddParent(gates_[parent]);
      gates_[parent]->AddChild(p_event);
      // Check for conditional event.
      if (type == "conditional" && gates_[parent]->type() != "inhibit") {
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
    if (gates_.count(id)) {
      // Doubly defined intermediate event.
      std::stringstream msg;
      msg << orig_ids_[id] << " intermediate event is doubly defined.";
      throw scram::ValidationError(msg.str());
    }
    // Detect name clashes.
    if (primary_events_.count(id)) {
      std::stringstream msg;
      msg << "The id " << orig_ids_[id]
          << " is already assigned to a primary event.";
      throw scram::ValidationError(msg.str());
    }

    GatePtr i_event(new scram::Gate(id));

    fault_tree_->AddGate(i_event);

    if (gates_.count(parent)) {
      i_event->AddParent(gates_[parent]);
      gates_[parent]->AddChild(i_event);
      gates_.insert(std::make_pair(id, i_event));
      all_events_.insert(std::make_pair(id, i_event));

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

void RiskAnalysis::AddProb(std::string id, double p) {
  // Check if the primary event is in this tree.
  if (!primary_events_.count(id)) {
    return;  // Ignore non-existent assignment.
  }

  primary_events_[id]->p(p);
}

void RiskAnalysis::AddProb(std::string id, double p, double time) {
  // Check if the primary event is in this tree.
  if (!primary_events_.count(id)) {
    return;  // Ignore non-existent assignment.
  }

  primary_events_[id]->p(p, time);
}

std::string RiskAnalysis::CheckAllGates() {
  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.

  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    msg << RiskAnalysis::CheckGate(it->second);
  }

  return msg.str();
}

std::string RiskAnalysis::CheckGate(const GatePtr& event) {
  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.
  try {
    std::string gate = event->type();
    // This line throws an error if there are no children.
    int size = event->children().size();

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
        std::map<std::string, EventPtr> children = event->children();
        std::map<std::string, EventPtr>::iterator it;
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

std::string RiskAnalysis::PrimariesNoProb() {
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
