// Implementation of fault tree analysis
#include "risk_analysis.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
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

FaultTree::FaultTree(std::string analysis, bool rare_event)
    : analysis_(analysis),
      rare_event_(rare_event),
      warnings_(""),
      top_event_id_(""),
      input_file_(""),
      p_total_(0) {
  // add valid gates
  gates_.insert("and");
  gates_.insert("or");

  // add valid primary event types
  types_.insert("basic");
  types_.insert("undeveloped");
  types_.insert("house");
}

void FaultTree::process_input(std::string input_file) {
  std::ifstream ifile(input_file.c_str());
  if (!ifile.good()) {
    std::string msg = "Input instruction file is not accessible.";
    throw(scram::IOError(msg));
  }
  input_file_ = input_file;

  std::string line = "";  // case insensitive input
  std::string orig_line = "";  // line with capitalizations preserved
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
            msg << "Line " << nline << " : " << "Found second '{' before"
                << " finishing the current block.";
            throw scram::ValidationError(msg.str());
          }
          // new block successfully started
          block_started = true;

        } else if (args[0] == "}") {
          // check if this is an end of a block
          if (!block_started) {
            std::stringstream msg;
            msg << "Line " << nline << " : " << " Found '}' before starting"
                << " a new block.";
            throw scram::ValidationError(msg.str());
          }
          // end of the block detected

          // Check if all needed arguments for an event are received.
          if (parent == "") {
            std::stringstream msg;
            msg << "Line " << nline << " : " << "Missing parent in this"
                << " block.";
            throw scram::ValidationError(msg.str());
          }

          if (id == "") {
            std::stringstream msg;
            msg << "Line " << nline << " : " << "Missing id in this"
                << " block.";
            throw scram::ValidationError(msg.str());
          }

          if (type == "") {
            std::stringstream msg;
            msg << "Line " << nline << " : " << "Missing type in this"
                << " block.";
            throw scram::ValidationError(msg.str());
          }

          try {
            // Add a node with the gathered information
            FaultTree::add_node_(parent, id, type);
          } catch (scram::ValidationError& err) {
            std::stringstream msg;
            msg << "Line " << nline << " : " << err.msg();
            throw scram::ValidationError(msg.str());
          }

          // refresh values
          parent = "";
          id = "";
          type = "";
          block_started = false;

        } else {
          std::stringstream msg;
          msg << "Line " << nline << " : " << "Undefined input.";
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
          if (!gates_.count(type) && !types_.count(type)) {
            std::stringstream msg;
            boost::to_upper(type);
            msg << "Line " << nline << " : " << "Invalid input arguments."
                << " Do not support this '" << type
                << "' gate/event type.";
            throw scram::ValidationError(msg.str());
          }
        } else {
          // There may go other parameters for FTA
          // For now, just throw an error
          std::stringstream msg;
          msg << "Line " << nline << " : " << "Invalid input arguments."
              << " Check spelling and doubly defined parameters inside"
              << " this block.";
          throw scram::ValidationError(msg.str());
        }

        break;
      }
      default: {
        std::stringstream msg;
        msg << "Line " << nline << " : " << "More than 2 arguments.";
        throw scram::ValidationError(msg.str());
      }
    }
  }

  // Defensive check if the top event has at least one child
  if (top_event_->children().size() == 0) {
    std::stringstream msg;
    msg << "The top event does not have intermediate or primary events.";
    throw scram::ValidationError(msg.str());
  }

  // Check if each gate has a right number of children
  std::string bad_gates = FaultTree::check_gates_();
  if (bad_gates != "") {
    std::stringstream msg;
    msg << "\nThere are problems with the following gates:\n";
    msg << bad_gates;
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

  std::string line = "";  // case insensitive input
  std::vector<std::string> no_comments;  // to hold lines without comments
  std::vector<std::string> args;  // to hold args after splitting the line

  std::string id = "";  // name id of a primary event
  double p = -1;  // probability of a primary event
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
            msg << "Line " << nline << " : " << "Found second '{' before"
                << " finishing the current block.";
            throw scram::ValidationError(msg.str());
          }
          // new block successfully started
          block_started = true;

        } else if (args[0] == "}") {
          // check if this is an end of a block
          if (!block_started) {
            std::stringstream msg;
            msg << "Line " << nline << " : " << " Found '}' before starting"
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
          msg << "Line " << nline << " : " << "Undefined input.";
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
          msg << "Line " << nline << " : " << "Incorrect probability input.";
          throw scram::ValidationError(msg.str());
        }

        try {
          // Add probability of a primary event
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
        msg << "Line " << nline << " : " << "More than 2 arguments.";
        throw scram::ValidationError(msg.str());
      }
    }
  }

  // Check if all primary events have probabilities initialized
  std::string uninit_primaries = FaultTree::primaries_no_prob_();
  if (uninit_primaries != "") {
    std::stringstream msg;
    msg << "Missing probabilities for following basic events:\n";
    msg << uninit_primaries;
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

  // container for cut sets with intermediate events
  std::vector< std::set<std::string> > inter_sets;

  // container for cut sets with primary events only
  std::vector< std::set<std::string> > cut_sets;

  // populate intermediate and primary events of the top
  std::map<std::string, scram::Event*> events_children = top_event_->children();

  // iterator for children of top and intermediate events
  std::map<std::string, scram::Event*>::iterator it_child;

  // Type dependent logic
  if (top_event_->gate() == "or") {
    for (it_child = events_children.begin();
         it_child != events_children.end(); ++it_child) {
      std::set<std::string> tmp_set_c;
      tmp_set_c.insert(it_child->first);
      inter_sets.push_back(tmp_set_c);
    }
  } else if (top_event_->gate() == "and") {
    std::set<std::string> tmp_set_c;
    for (it_child = events_children.begin();
         it_child != events_children.end(); ++it_child) {
      tmp_set_c.insert(it_child->first);
    }
    inter_sets.push_back(tmp_set_c);
  } else {
    std::string msg = "No algorithm defined for" + top_event_->gate();
    throw scram::ValueError(msg);
  }

  // an iterator for a set with ids of events
  std::set<std::string>::iterator it_set;

  // an iterator for a vector with sets of ids of events
  std::vector< std::set<std::string> >::iterator it_vec;

  // Generate cut sets
  while (!inter_sets.empty()) {
    // get rightmost set
    std::set<std::string> tmp_set = inter_sets.back();
    // delete rightmost set
    inter_sets.pop_back();
    // iterate till finding intermediate event
    std::string first_inter_event = "";
    for (it_set = tmp_set.begin(); it_set != tmp_set.end(); ++it_set) {
      if (inter_events_.count(*it_set)) {
        first_inter_event = *it_set;
        // clear temp set from the found intermediate event
        tmp_set.erase(it_set);

        break;
      }
    }

    if (first_inter_event == "") {
      // this is a set with primary events only
      cut_sets.push_back(tmp_set);
      continue;
    }

    // get the intermediate event
    scram::InterEvent* inter_event = inter_events_[first_inter_event];
    events_children = inter_event->children();
    // to hold sets of children
    std::vector< std::set<std::string> > children_sets;

    // Type dependent logic
    if (inter_event->gate() == "or") {
      for (it_child = events_children.begin();
           it_child != events_children.end(); ++it_child) {
        std::set<std::string> tmp_set_c;
        tmp_set_c.insert(it_child->first);
        children_sets.push_back(tmp_set_c);
      }
    } else if (inter_event->gate() == "and") {
      std::set<std::string> tmp_set_c;
      for (it_child = events_children.begin();
           it_child != events_children.end(); ++it_child) {
        tmp_set_c.insert(it_child->first);
      }
      children_sets.push_back(tmp_set_c);
    } else {
      std::string msg = "No algorithm defined for" + inter_event->gate();
      throw scram::ValueError(msg);
    }

    // attach the original set into this event's sets of children
    for (it_vec = children_sets.begin(); it_vec != children_sets.end();
         ++it_vec) {
      it_vec->insert(tmp_set.begin(), tmp_set.end());
      // add this set to the original inter_sets
      inter_sets.push_back(*it_vec);
    }
  }

  // At this point cut sets are generated.
  // Now we need to reduce them to minimal cut sets.
  // Choose to convert vector to a set to get rid of any duplications
  std::set< std::set<std::string> > unique_cut_sets;
  for (it_vec = cut_sets.begin(); it_vec != cut_sets.end(); ++it_vec) {
    if (it_vec->size() == 1) {
      // Minimal cut set is detected
      min_cut_sets_.insert(*it_vec);
      continue;
    }
    unique_cut_sets.insert(*it_vec);
  }
  // iterator for unique_cut_sets
  std::set< std::set<std::string> >::iterator it_uniq;

  // iterator for minimal cut sets
  std::set< std::set<std::string> >::iterator it_min;

  // minimal size of sets in uniq_cut_sets
  int min_size = 2;

  while (!unique_cut_sets.empty()) {
    // Apply rule 4 to reduce unique cut sets

    std::set< std::set<std::string> > temp_sets;

    for (it_uniq = unique_cut_sets.begin();
         it_uniq != unique_cut_sets.end(); ++it_uniq) {

         bool include = true;  // determine to keep or not

      for (it_min = min_cut_sets_.begin();
           it_min != min_cut_sets_.end(); ++it_min) {
        if (std::includes(it_uniq->begin(), it_uniq->end(),
                          it_min->begin(), it_min->end())) {
          // non-minimal cut set is detected
          include = false;
          break;
        }
      }
      // after checking for non-minimal cut sets
      // all minimum sized cut sets are guaranteed to be minimal
      if (include) {
        if (it_uniq->size() == min_size) {
          min_cut_sets_.insert(*it_uniq);
        } else {
          temp_sets.insert(*it_uniq);
        }
      }
      // ignore the cut set because include = false
    }
    unique_cut_sets = temp_sets;
    min_size++;
  }

  // Compute probabilities
  // First, assume independence of events.
  // Second, rare event approximation is applied upon users' request

  // Iterate minimal cut sets and find probabilities for each set
  for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
        ++it_min) {
    // calculate a probability of a set with AND relationship
    double p_sub_set = FaultTree::prob_and_(*it_min);
    // update a container with minimal cut sets and probabilities
    prob_of_min_sets_.insert(std::make_pair(*it_min, p_sub_set));
  }

  // Check if a rare event approximation is requested
  if (rare_event_) {
    warnings_ += "Using the rare event approximation\n";
    bool rare_event_legit = true;
    std::map< std::set<std::string>, double >::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin();
         it_pr != prob_of_min_sets_.end(); ++it_pr) {
      // check if a probability of a set does not exceed 0.1,
      // which is required for the rare event approximation to hold.
      if (rare_event_legit && (it_pr->second > 0.1)) {
        rare_event_legit = false;
        warnings_ += "The rare event approximation may be inaccurate for this"
            "\nfault tree analysis because one of minimal cut sets'"
            "\nprobability exceeded 0.1 threshold requirement.\n\n";
      }
      p_total_ += it_pr->second;
    }
  } else {
    // Exact calculation of probability of cut sets
    p_total_ = prob_or_(min_cut_sets_);
  }

  // check if total probability is above 1
  if (p_total_ > 1) {
    warnings_ += "The rare event approximation does not hold. Total probability"
                 "\nis above 1. Switching to the brute force algorithm.\n";
    // Re-calculate total probability
    p_total_ = prob_or_(min_cut_sets_);
  }

  // Calculate probability of each minimal cut set for further analysis

}

void FaultTree::report(std::string output) {
  // Check if output to file is requested
  std::streambuf* buf;
  std::ofstream of;
  if (output != "cli") {
    of.open(output.c_str());
    buf = of.rdbuf();
  } else {
    buf = std::cout.rdbuf();
  }
  std::ostream out(buf);

  // an iterator for a set with ids of events
  std::set<std::string>::iterator it_set;

  // iterator for minimal cut sets
  std::set< std::set<std::string> >::iterator it_min;

  // iterator for a map with minimal cut sets and their probabilities
  std::map< std::set<std::string>, double >::iterator it_pr;

  // Print warnings of calculations
  out << "\n" << warnings_ << "\n";

  // Print minimal cut sets
  out << "\n" << "Begin minimal cut sets" << "\n";
  for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
       ++it_min) {
    out << "{ ";
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      out << orig_ids_[*it_set] << " ";
    }
    out << "}\n";
    out.flush();
  }

  // Print probabilities of minimal cut sets
  out << "\n" << "Begin minimal cut sets' probabilities\n";
  for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
       ++it_min) {
    out << "{ ";
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      out << orig_ids_[*it_set] << " ";
    }
    out << "}    ";
    out << prob_of_min_sets_[*it_min] << "\n";
    out.flush();
  }

  // Print total probability
  out << "\n" << "Total probability of this fault tree" << "\n";
  out << p_total_ << "\n";

}

void FaultTree::add_node_(std::string parent, std::string id,
                          std::string type) {
  // Initialize events
  if (parent == "none") {
    // check for inaccurate second top event
    if (top_event_id_ == "") {
      top_event_id_ = id;
      top_event_ = new scram::TopEvent(top_event_id_);

      // top event cannot be primary
      if (!gates_.count(type)) {
        std::stringstream msg;
        msg << "Invalid input arguments. " << orig_ids_[top_event_id_]
            << " top event type is not defined correctly. It must be a gate.";
        throw scram::ValidationError(msg.str());
      }

      top_event_->gate(type);

    } else {
      // Another top event is detected
      std::stringstream msg;
      msg << "Invalid input arguments. The input cannot contain more than"
          << " one top event. " << orig_ids_[id] << " is redundant.";
      throw scram::ValidationError(msg.str());
    }

  } else if (types_.count(type)) {
    // this must be a primary event
    scram::PrimaryEvent* p_event = new PrimaryEvent(id, type);
    if (parent == top_event_id_){
      p_event->add_parent(top_event_);
      top_event_->add_child(p_event);
      primary_events_.insert(std::make_pair(id, p_event));

    } else if (inter_events_.count(parent)) {
      p_event->add_parent(inter_events_[parent]);
      inter_events_[parent]->add_child(p_event);
      primary_events_.insert(std::make_pair(id, p_event));

    } else {
      // parent is undefined
      std::stringstream msg;
      msg << "Invalid input arguments. Parent of " << orig_ids_[id]
          << " primary event is not defined.";
      throw scram::ValidationError(msg.str());
    }

  } else {
    // this must be a new intermediate event
    if (inter_events_.count(id)) {
      // doubly defined intermediate event
      std::stringstream msg;
      msg << orig_ids_[id] << " intermediate event is doubly defined.";
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
      msg << "Invalid input arguments. Parent of " << orig_ids_[id]
          << " intermediate event is not defined.";
      throw scram::ValidationError(msg.str());
    }

    i_event -> gate(type);
  }

  // check if a top event defined the first
  if (top_event_id_ == "") {
    std::stringstream msg;
    msg << "Invalid input arguments. Top event must be defined first.";
    throw scram::ValidationError(msg.str());
  }
}

void FaultTree::add_prob_(std::string id, double p) {
  // Check if the primary event is in this tree
  if (primary_events_.count(id) == 0) {
    std::string msg = "Primary event " + id + " was not initiated in this tree.";
    throw scram::ValidationError(msg);
  }

  primary_events_[id]->p(p);

}

std::string FaultTree::check_gates_() {
  std::stringstream msg;
  msg << "";  // an empty default message, an indicator of no problems
  std::map<std::string, scram::InterEvent*>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    try {
      std::string gate = it->second->gate();
      // this line throws error if there are no children
      int size = it->second->children().size();

      // gate dependent logic
      if ((gate == "and") && (size < 2)) {
        msg << orig_ids_[it->first] << " : AND gate must have 2 or more "
            << "children.\n";
      } else if ((gate == "or") && (size < 2)) {
        msg << orig_ids_[it->first] << " : OR gate must have 2 or more "
            << "children.\n";
      }

    } catch (scram::ValueError& err) {
      msg << orig_ids_[it->first] << " : No children detected.";
    }
  }

  return msg.str();
}

std::string FaultTree::primaries_no_prob_() {
  std::string uninit_primaries = "";
  std::map<std::string, scram::PrimaryEvent*>::iterator it;
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

double FaultTree::prob_or_(std::set< std::set<std::string> > min_cut_sets_) {
  // Recursive implementation
  if (min_cut_sets_.empty()) {
    throw scram::ValueError("Do not pass empty set to prob_or_ function.");
  }

  double prob = 0;

  // Get one element
  std::set< std::set<std::string> >::iterator it = min_cut_sets_.begin();
  // it++;  // advance to the first element
  std::set<std::string> element_one = *it;

  // Base case
  if (min_cut_sets_.size() == 1) {
    // Get only element in this set
    return FaultTree::prob_and_(element_one);
  }

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets_.erase(it);
  prob = FaultTree::prob_and_(element_one) +
            FaultTree::prob_or_(min_cut_sets_) -
            FaultTree::prob_or_(FaultTree::combine_el_and_set_(element_one,
                                                               min_cut_sets_));

  return prob;
}

double FaultTree::prob_and_(const std::set< std::string>& min_cut_set) {
  // Test just in case the min cut set is empty
  if (min_cut_set.empty()) {
    throw scram::ValueError("The set is empty for probability calculations.");
  }

  double p_sub_set = 1;  // 1 is for multiplication
  std::set<std::string>::iterator it_set;
  for (it_set = min_cut_set.begin(); it_set != min_cut_set.end(); ++it_set) {
    p_sub_set *= primary_events_[*it_set]->p();
  }
  return p_sub_set;
}

std::set< std::set<std::string> > FaultTree::combine_el_and_set_(
    const std::set< std::string>& el,
    const std::set< std::set<std::string> >& set) {

  std::set< std::set<std::string> > combo_set;
  std::set< std::string> member_set;
  std::set< std::set<std::string> >::iterator it_set;
  for (it_set = set.begin(); it_set != set.end(); ++it_set) {
    member_set = *it_set;
    member_set.insert(el.begin(), el.end());
    combo_set.insert(member_set);
  }

  return combo_set;

}

  bool pair_comp_(const std::pair< std::set<std::string>, double >& m_one,
                  const std::pair< std::set<std::string>, double >& m_two) {
    return m_one.second < m_two.second;
  }

}  // namespace scram
