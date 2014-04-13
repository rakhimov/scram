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
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>

#include "error.h"
#include "event.h"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace scram {

FaultTree::FaultTree(std::string analysis, bool rare_event, int limit_order,
                     int nsums)
    : analysis_(analysis),
      rare_event_(rare_event),
      limit_order_(limit_order),
      nsums_(nsums),
      warnings_(""),
      top_event_id_(""),
      top_detected_(false),
      is_main_(true),
      input_file_(""),
      prob_requested_(false),
      max_order_(1),
      p_total_(0),
      parent_(""),
      id_(""),
      type_(""),
      block_started_(false),
      transfer_correct_(false),
      transfer_first_inter_(false) {
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
    std::string msg = input_file +  " : Tree file is not accessible.";
    throw(scram::IOError(msg));
  }
  input_file_ = input_file;

  std::string line = "";  // case insensitive input
  std::string orig_line = "";  // line with capitalizations preserved
  std::vector<std::string> args;  // to hold args after splitting the line
  std::vector<std::string> orig_args;  // original input with capitalizations

  // Error messages
  std::stringstream msg;

  for (int nline = 1; getline(ifile, line); ++nline) {
    if (!FaultTree::get_args_(args, line, orig_line)) continue;

    FaultTree::interpret_args_(nline, msg, args, orig_line);
  }

  // Transfer include external trees from other files
  if (!transfers_.empty()) {
    FaultTree::include_transfers_();
  }

  // Defensive check if the top event has at least one child
  if (top_event_->children().empty()) {
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
    std::string msg = prob_file + " : Probability file is not accessible.";
    throw(scram::IOError(msg));
  }

  prob_requested_ = true;  // switch indicator

  std::string line = "";  // case insensitive input
  std::string orig_line = "";  // original line
  std::vector<std::string> args;  // to hold args after splitting the line

  std::string id = "";  // name id of a primary event
  double p = -1;  // probability of a primary event
  bool block_started = false;

  // Error messages
  std::stringstream msg;

  for (int nline = 1; getline(pfile, line); ++nline) {
    if (!FaultTree::get_args_(args, line, orig_line)) continue;

    switch (args.size()) {
      case 1: {
        if (args[0] == "{") {
          // check if this is a new start of a block
          if (block_started) {
            msg << "Line " << nline << " : " << "Found second '{' before"
                << " finishing the current block.";
            throw scram::ValidationError(msg.str());
          }
          // new block successfully started
          block_started = true;

        } else if (args[0] == "}") {
          // check if this is an end of a block
          if (!block_started) {
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
          msg << "Line " << nline << " : " << "Incorrect probability input.";
          throw scram::ValidationError(msg.str());
        }

        try {
          // Add probability of a primary event
          FaultTree::add_prob_(id, p);
        } catch (scram::ValidationError& err_1) {
          msg << "Line " << nline << " : " << err_1.msg();
          throw scram::ValidationError(msg.str());
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

  // Check if all primary events have probabilities initialized
  std::string uninit_primaries = FaultTree::primaries_no_prob_();
  if (uninit_primaries != "") {
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

  FaultTree::expand_sets_(top_event_, inter_sets);

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
      // discard this tmp set if it is larger than the limit
      if (tmp_set.size() > limit_order_) continue;

      // this is a set with primary events only
      cut_sets.push_back(tmp_set);
      continue;
    }

    // get the intermediate event
    scram::InterEvent* inter_event = inter_events_[first_inter_event];
    // to hold sets of children
    std::vector< std::set<std::string> > children_sets;

    FaultTree::expand_sets_(inter_event, children_sets);

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

  // First, defensive check if cut sets exists for the specified limit order
  if (cut_sets.empty()) {
    std::stringstream msg;
    msg << "No cut sets for the limit order " << limit_order_;
    throw scram::ValueError(msg.str());
  }

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
          // update maximum order of the sets
          if (min_size > max_order_) max_order_ = min_size;
        } else {
          temp_sets.insert(*it_uniq);
        }
      }
      // ignore the cut set because include = false
    }
    unique_cut_sets = temp_sets;
    min_size++;
  }

  // Compute probabilities only if requested
  if (!prob_requested_) return;

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
    p_total_ = prob_or_(min_cut_sets_, nsums_);
  }

  // check if total probability is above 1
  if (p_total_ > 1) {
    warnings_ += "The rare event approximation does not hold. Total probability"
                 "\nis above 1. Switching to the brute force algorithm.\n";
    // Re-calculate total probability
    p_total_ = prob_or_(min_cut_sets_, nsums_);
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
  if (warnings_ != "") {
    out << "\n" << warnings_ << "\n";
  }

  // Print minimal cut sets by their order
  out << "\n" << "Minimal Cut Sets" << "\n";
  out << "================\n\n";
  out << "Fault Tree: " << input_file_ << "\n";
  out << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << "Analysis algorithm: " << analysis_ << "\n";
  out << "Limit on order of cut sets: " << limit_order_ << "\n";
  out << "Number of Primary Events: " << primary_events_.size() << "\n";
  out << "Minimal Cut Set Maximum Order: " << max_order_ << "\n";
  out.flush();

  int order = 1;  // order of minimal cut sets
  std::vector<int> order_numbers;  // number of sets per order
  while (order < max_order_ + 1) {
    std::set< std::set<std::string> > order_sets;
    for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
        ++it_min) {
      if (it_min->size() == order) {
        order_sets.insert(*it_min);
      }
    }
    order_numbers.push_back(order_sets.size());
    if (!order_sets.empty()) {
      out << "\nOrder " << order << ":\n";
      int i = 1;
      for (it_min = order_sets.begin(); it_min != order_sets.end(); ++it_min) {
        out << i << ") ";
        out << "{ ";
        for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
          out << orig_ids_[*it_set] << " ";
        }
        out << "}\n";
        out.flush();
        i++;
      }
    }
    order++;
  }

  out << "\nQualitative Importance Analysis:" << "\n\n";
  out << "Order        Number\n";
  out << "-----        ------\n";
  for (int i = 1; i < max_order_ + 1; ++i) {
    out << "  " << i << "            " << order_numbers[i-1] << "\n";
  }
  out.flush();

  // Print probabilities of minimal cut sets only if requested
  if (!prob_requested_) return;

  out << "\n" << "Probability Analysis" << "\n";
  out << "====================\n\n";
  out << "Fault Tree: " << input_file_ << "\n";
  out << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << "Analysis type: " << analysis_ << "\n";
  out << "Limit on series: " << nsums_ << "\n";
  out << "Number of Primary Events: " << primary_events_.size() << "\n";
  out << "Number of Minimal Cut Sets: " << min_cut_sets_.size() << "\n\n";
  out << "Minimal Cut Set Probabilities:\n\n";
  out.flush();
  int i = 1;
  for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
       ++it_min) {
    out << i << ") { ";
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      out << orig_ids_[*it_set] << " ";
    }
    out << "}    ";
    out << prob_of_min_sets_[*it_min] << "\n";
    i++;
    out.flush();
  }

  // Print total probability
  out << "\n" << "=============================\n";
  out <<  "Total Probability: " << p_total_ << "\n";
  out << "=============================\n\n";
  out.flush();
}

bool FaultTree::get_args_(std::vector<std::string>& args, std::string& line,
                          std::string& orig_line) {
    std::vector<std::string> no_comments;  // to hold lines without comments

    // remove trailing spaces
    boost::trim(line);
    // check if line is empty
    if (line == "") return false;

    // remove comments starting with # sign
    boost::split(no_comments, line, boost::is_any_of("#"),
                 boost::token_compress_on);
    line = no_comments[0];

    // check if line is comment only
    if (line == "") return false;

    // trim again for spaces before in-line comments
    boost::trim(line);

    // preserve original line for name extraction
    orig_line = line;

    // convert to lower case
    boost::to_lower(line);

    // get args from the line
    boost::split(args, line, boost::is_any_of(" "),
                 boost::token_compress_on);
    return true;
}

void FaultTree::interpret_args_(int nline, std::stringstream& msg,
                                std::vector<std::string>& args,
                                std::string& orig_line,
                                std::string tr_parent,
                                std::string tr_id,
                                std::string suffix) {
  std::vector<std::string> orig_args;  // original input with capitalizations

  switch (args.size()) {
    case 1: {
      if (args[0] == "{") {
        // check if this is a new start of a block
        if (block_started_) {
          msg << "Line " << nline << " : " << "Found second '{' before"
              << " finishing the current block.";
          throw scram::ValidationError(msg.str());
        }
        // new block successfully started
        block_started_ = true;

      } else if (args[0] == "}") {
        // check if this is an end of a block
        if (!block_started_) {
          msg << "Line " << nline << " : " << " Found '}' before starting"
              << " a new block.";
          throw scram::ValidationError(msg.str());
        }
        // end of the block detected

        // Check if all needed arguments for an event are received.
        if (parent_ == "") {
          msg << "Line " << nline << " : " << "Missing parent in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        }

        if (id_ == "") {
          msg << "Line " << nline << " : " << "Missing id in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        }

        if (type_ == "") {
          msg << "Line " << nline << " : " << "Missing type in this"
              << " block.";
          throw scram::ValidationError(msg.str());
        }

        if (!is_main_) {
          // Inidication that this is a transfer subtree
          // Check for correct transfer initialization
          if (!transfer_correct_) {
            if (parent_ != "any") {
              msg << "Line " << nline << " : Parent of TransferOut must be "
                  << "'Any'.";
              throw scram::ValidationError(msg.str());
            } else if (id_ != tr_id) {
              msg << "Line " << nline << " : Id of the first node in "
                  << "TransferOut must match the name of the file.";
              throw scram::ValidationError(msg.str());
            } else if (type_ != "transferout") {
              msg << "Line " << nline << " : Type of the first node in "
                  << "Transfered tree must be 'TransferOut'";
              throw scram::ValidationError(msg.str());
            }
            transfer_correct_ = true;
            // refresh values
            parent_ = "";
            id_ = "";
            type_ = "";
            block_started_ = false;
            break;
          }

          // Check if this is the main intermediate event in the transfer tree
          if (!transfer_first_inter_) {
            if (parent_ != tr_id) {
              msg << "Line " << nline << " : Parent of the first intermediate "
                  << "event must be the name of the file.";
              throw scram::ValidationError(msg.str());
            } else if (types_.count(type_)) {
              msg << "Line " << nline << " : Type of the first intermediate "
                  << "event in the transfer tree cannot be a primary event.";
              throw scram::ValidationError(msg.str());
            }
            // re-assign parent of the first intermediate event in this
            // sub-tree to the parent of the transfer in order to link
            // trees
            parent_ = tr_parent;
            transfer_first_inter_ = true;
          } else {
            // Attach specific suffix for the parents of events
            // in the sub-tree
            parent_ += suffix;
          }

          // Defensive check if there are other events having the name of
          // the current transfer file as their parent
          if (parent_ == tr_id) {
            msg << "Line " << nline << " : Found the second event with "
                << "a parent as a starting node. Only one intermediate event"
                << " can be linked to TransferOut of the transfer tree.";
            throw scram::ValidationError(msg.str());
          }

          // Defensive check if there is another TransferOut in the same file
          if (type_ == "transferout") {
            msg << "Line " << nline << " : Found the second TransferOut in "
                << "the same transfer sub-tree definition.";
            throw scram::ValidationError(msg.str());
          }

          // Attach specific suffix for the intermediate events
          if (gates_.count(type_)) {
            id_ += suffix;
          }
        }

        try {
          // Add a node with the gathered information
          FaultTree::add_node_(parent_, id_, type_);
        } catch (scram::ValidationError& err) {
          msg << "Line " << nline << " : " << err.msg();
          throw scram::ValidationError(msg.str());
        }

        // refresh values
        parent_ = "";
        id_ = "";
        type_ = "";
        block_started_ = false;

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
        // extract and save original id with capitalizations
        // get args from the original line
        boost::split(orig_args, orig_line, boost::is_any_of(" "),
                     boost::token_compress_on);
        // populate names
        orig_ids_.insert(std::make_pair(id_, orig_args[1]));

      } else if (args[0] == "type" && type_ == "") {
        type_ = args[1];
        // check if gate/event type is valid
        if (!gates_.count(type_) && !types_.count(type_)
            && (type_ != "transferin")
            && (type_ != "transferout")) {
          boost::to_upper(type_);
          msg << "Line " << nline << " : " << "Invalid input arguments."
              << " Do not support this '" << type_
              << "' gate/event type.";
          throw scram::ValidationError(msg.str());
        }
      } else {
        // There may go other parameters for FTA
        // For now, just throw an error
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

void FaultTree::add_node_(std::string parent, std::string id,
                          std::string type) {
  // Check if this is a transfer
  if (type == "transferin") {
    if (parent == "none") {
      top_detected_ = true;
    }
    // register to call later
    transfers_.push(std::make_pair(parent, id));
    trans_calls_.insert(std::make_pair(id, 0));
    // check if this is a cyclic inclusion
    // this line does many things that are tricky and c++ map specific.
    // find vectors ending with the currently opened file name and append
    // the sub-tree requested to be called later.
    // Attach that subtree if it does not clash with the initiating grandparents

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

  // Initialize events
  if (parent == "none") {
    if (top_detected_ && is_main_) {
      std::stringstream msg;
      msg << "Found a second top event in file where top event is described "
          << "by a transfer sub-tree.";
      throw scram::ValidationError(msg.str());
    }

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
    if (parent == top_event_id_) {
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
    boost::to_upper(id);
    std::string msg = "Primary event " + id + " is not in this tree.";
    throw scram::ValidationError(msg);
  }

  primary_events_[id]->p(p);
}

void FaultTree::include_transfers_() {
  is_main_ = false;
  while (!transfers_.empty()) {
    std::pair<std::string, std::string> tr_node = transfers_.front();
    transfers_.pop();

    // parent of this transfer symbol
    std::string tr_parent = tr_node.first;
    // id of this transfer symbol, which is the name of the file
    std::string tr_id = tr_node.second;
    // increase the number of calls of this sub-tree
    trans_calls_[tr_id] += 1;
    // an attachment to intermediate event names in order to avoid clashes
    std::stringstream sufstr;
    sufstr << "-" << tr_id << "[" << trans_calls_[tr_id] << "]";
    std::string suffix = sufstr.str();

    // The sub-tree descriptions must be located in the same directory as
    // the main input file.
    fs::path p(input_file_);
    std::string dir = p.parent_path().generic_string();
    std::string path_to_tr = "";
    if (dir != "") {
      path_to_tr = dir + "/" + tr_id;
    } else {
      path_to_tr = tr_id;
    }

    std::ifstream ifile(path_to_tr.c_str());
    if (!ifile.good()) {
      std::string msg = tr_id + " : tree file is not accessible.";
      throw(scram::IOError(msg));
    }

    // register that a sub-tree file is under operation
    current_file_ = tr_id;

    std::string line = "";  // case insensitive input
    std::string orig_line = "";  // line with capitalizations preserved
    std::vector<std::string> args;  // to hold args after splitting the line
    std::vector<std::string> orig_args;  // original input with capitalizations

    // Error messages
    std::stringstream msg;
    // File specific indication for errors
    msg << "In " << tr_id << ", ";

    // indicate if TransferOut is initiated correctly
    transfer_correct_ = false;

    // indication of the first intermediate event of the transfer
    transfer_first_inter_ = false;

    for (int nline = 1; getline(ifile, line); ++nline) {
      if (!FaultTree::get_args_(args, line, orig_line)) continue;

      FaultTree::interpret_args_(nline, msg,  args, orig_line, tr_parent,
                                 tr_id, suffix);
    }
  }
}

void FaultTree::expand_sets_(scram::TopEvent* t,
                             std::vector< std::set<std::string> >& sets) {
  // populate intermediate and primary events of the top
  std::map<std::string, scram::Event*> events_children = t->children();

  // iterator for children of top and intermediate events
  std::map<std::string, scram::Event*>::iterator it_child;

  // Type dependent logic
  if (t->gate() == "or") {
    for (it_child = events_children.begin();
         it_child != events_children.end(); ++it_child) {
      std::set<std::string> tmp_set_c;
      tmp_set_c.insert(it_child->first);
      sets.push_back(tmp_set_c);
    }
  } else if (t->gate() == "and") {
    std::set<std::string> tmp_set_c;
    for (it_child = events_children.begin();
         it_child != events_children.end(); ++it_child) {
      tmp_set_c.insert(it_child->first);
    }
    sets.push_back(tmp_set_c);
  } else {
    std::string msg = "No algorithm defined for" + t->gate();
    throw scram::ValueError(msg);
  }
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

double FaultTree::prob_or_(std::set< std::set<std::string> > min_cut_sets,
                           int nsums) {
  // Recursive implementation
  if (min_cut_sets.empty()) {
    throw scram::ValueError("Do not pass empty set to prob_or_ function.");
  }

  if (nsums == 0) {
    return 0;
  }

  double prob = 0;

  // Get one element
  std::set< std::set<std::string> >::iterator it = min_cut_sets.begin();
  std::set<std::string> element_one = *it;

  // Base case
  if (min_cut_sets.size() == 1) {
    nsums--;
    // Get only element in this set
    return FaultTree::prob_and_(element_one);
  }

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets.erase(it);
  prob = FaultTree::prob_and_(element_one) +
         FaultTree::prob_or_(min_cut_sets, nsums) -
         FaultTree::prob_or_(FaultTree::combine_el_and_set_(element_one,
                                                            min_cut_sets),
                             nsums - 1);
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

}  // namespace scram
