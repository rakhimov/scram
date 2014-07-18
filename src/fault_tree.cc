// Implementation of fault tree analysis.
#include "fault_tree.h"

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


namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace scram {

FaultTree::FaultTree(std::string analysis, bool graph_only, bool rare_event,
                     int limit_order, int nsums)
    : analysis_(analysis),
      rare_event_(rare_event),
      limit_order_(limit_order),
      graph_only_(graph_only),
      nsums_(nsums),
      warnings_(""),
      top_event_id_(""),
      top_detected_(false),
      is_main_(true),
      sub_tree_analysis_(false),
      input_file_(""),
      prob_requested_(false),
      analysis_done_(false),
      max_order_(1),
      p_total_(0),
      parent_(""),
      id_(""),
      type_(""),
      block_started_(false),
      transfer_correct_(false),
      transfer_first_inter_(false) {
  // Add valid gates.
  gates_.insert("and");
  gates_.insert("or");

  // Add valid primary event types.
  types_.insert("basic");
  types_.insert("undeveloped");
  types_.insert("house");

  // Pointer to the top event
  TopEventPtr top_event_;
}

void FaultTree::ProcessInput(std::string input_file) {
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
    if (!FaultTree::GetArgs_(args, line, orig_line)) continue;

    FaultTree::InterpretArgs_(nline, msg, args, orig_line);
  }

  // Check if the last data is written, and the last '}' isn't omitted.
  if (block_started_) {
    msg << "Missing closing '}' at the end of a file.";
    throw scram::ValidationError(msg.str());
  }

  // Transfer include external trees from other files.
  if (!transfers_.empty() && !graph_only_) {
    FaultTree::IncludeTransfers_();
  }

  // Check if all gates have a right number of children.
  std::string bad_gates = FaultTree::CheckAllGates_();
  if (bad_gates != "") {
    std::stringstream msg;
    msg << "\nThere are problems with the following gates:\n";
    msg << bad_gates;
    throw scram::ValidationError(msg.str());
  }
}

void FaultTree::PopulateProbabilities(std::string prob_file) {
  // Check if input file with tree instructions has already been read.
  if (input_file_ == "") {
    std::string msg = "Read input file with tree instructions before attaching"
                      " probabilities.";
    throw scram::Error(msg);
  }

  std::ifstream pfile(prob_file.c_str());
  if (!pfile.good()) {
    std::string msg = prob_file + " : Probability file is not accessible.";
    throw(scram::IOError(msg));
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
    if (!FaultTree::GetArgs_(args, line, orig_line)) continue;

    switch (args.size()) {
      case 1: {
        if (args[0] == "{") {
          // Check if this is a new start of a block.
          if (block_started) {
            msg << "Line " << nline << " : " << "Found second '{' before"
                << " finishing the current block.";
            throw scram::ValidationError(msg.str());
          }
          // New block successfully started.
          block_started = true;

        } else if (args[0] == "}") {
          // Check if this is an end of a block.
          if (!block_started) {
            msg << "Line " << nline << " : " << " Found '}' before starting"
                << " a new block.";
            throw scram::ValidationError(msg.str());
          }
          // End of the block detected.
          // Refresh values.
          id = "";
          p = -1;
          time = -1;
          block_started = false;
          block_type = "p-model";
          block_set = false;

        } else {
          msg << "Line " << nline << " : " << "Undefined input.";
          throw scram::ValidationError(msg.str());
        }

        break;
      }
      case 2: {
        if (!block_started) {
          msg << "Line " << nline << " : " << "Missing opening bracket {";
          throw scram::ValidationError(msg.str());
        }

        id = args[0];

        if (id == "block") {
          if (!block_set) {
            block_type = args[1];
            if (block_type != "p-model" && block_type != "l-model") {
              msg << "Line " << nline << " : " << "Unrecognized block type.";
              throw scram::ValidationError(msg.str());
            }
            block_set = true;
            break;
          } else  {
            msg << "Line " << nline << " : " << "Doubly defining this block.";
            throw scram::ValidationError(msg.str());
          }
        }

        if (id == "time" && block_type == "l-model") {
          if (time == -1) {
            try {
              time = boost::lexical_cast<double>(args[1]);
            } catch (boost::bad_lexical_cast err) {
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
        } catch (boost::bad_lexical_cast err) {
          msg << "Line " << nline << " : " << "Incorrect probability input.";
          throw scram::ValidationError(msg.str());
        }

        try {
          // Add probability of a primary event.
          if (block_type == "p-model") {
            FaultTree::AddProb_(id, p);
          } else if (block_type == "l-model") {
            if (time == -1) {
              msg << "Line " << nline << " : " << "L-Model Time is not given";
              throw scram::ValidationError(msg.str());
            }
            FaultTree::AddProb_(id, p, time);
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

  // Check if the last data is written, and the last '}' isn't omitted.
  if (block_started) {
    msg << "Missing closing '}' at the end of a file.";
    throw scram::ValidationError(msg.str());
  }

  // Check if all primary events have probabilities initialized.
  std::string uninit_primaries = FaultTree::PrimariesNoProb_();
  if (uninit_primaries != "") {
    msg << "Missing probabilities for following basic events:\n";
    msg << uninit_primaries;
    throw scram::ValidationError(msg.str());
  }
}

void FaultTree::GraphingInstructions() {
  // Check if input file with tree instructions has already been read.
  if (input_file_ == "") {
    std::string msg = "Read input file with tree instructions and initialize"
                      " the tree before requesting for a graphing"
                      " instructions.";
    throw scram::Error(msg);
  }

  // List inter events and their children.
  // List inter events and primary events' descriptions.
  std::string graph_name = input_file_;
  graph_name.erase(graph_name.find_last_of("."), std::string::npos);

  std::string output_path = graph_name + ".dot";

  graph_name = graph_name.substr(graph_name.find_last_of("/") +
                                 1, std::string::npos);
  std::ofstream out(output_path.c_str());
  if (!out.good()) {
    std::string msg = output_path +  " : Cannot write the graphing file.";
    throw(scram::IOError(msg));
  }

  // Check for the special case when only one node TransferIn tree is graphed.
  if (top_event_id_ == "") {
    std::string msg = "Cannot graph one node TransferIn tree.";
    throw(scram::ValidationError(msg));
  }

  boost::to_upper(graph_name);
  out << "digraph " << graph_name << " {\n";
  // Special case for sub-tree only graphing.
  if (sub_tree_analysis_) {
    std::string tr_id =
        input_file_.substr(input_file_.find_last_of("/") +
                           1, std::string::npos);
    out << "\"" << tr_id << "\" -> "
        << "\"" << orig_ids_[top_event_id_] <<"\";\n";
    out << "\"" << tr_id << "\" [shape=invtriangle, "
        << "fontsize=10, fontcolor=black, "
        << "label=\"" << tr_id << "\"]\n";
  }

  // Write top event.
  // Keep track of number of repetitions of the primary events.
  std::map<std::string, int> pr_repeat;
  // Populate intermediate and primary events of the top.
  FaultTree::GraphNode_(top_event_, pr_repeat, out);
  out.flush();
  // Do the same for all intermediate events.
  boost::unordered_map<std::string, InterEventPtr>::iterator it_inter;
  for (it_inter = inter_events_.begin(); it_inter != inter_events_.end();
       ++it_inter) {
    FaultTree::GraphNode_(it_inter->second, pr_repeat, out);
    out.flush();
  }

  // Do the same for all transfers.
  std::pair<std::string, std::string> tr_pair;
  while (!transfers_.empty()) {
    tr_pair = transfers_.front();
    transfers_.pop();
    out << "\"" <<  orig_ids_[tr_pair.first] << "\" -> "
        << "\"" << orig_ids_[tr_pair.second] <<"\";\n";
    // Apply format.
    std::string tr_name = orig_ids_[tr_pair.second];
    tr_name = tr_name.substr(tr_name.find_last_of("/") + 1, std::string::npos);
    out << "\"" << orig_ids_[tr_pair.second] << "\" [shape=triangle, "
        << "fontsize=10, fontcolor=black, fontname=\"times-bold\", "
        << "label=\"" << tr_name << "\"]\n";
  }

  // Format events.
  std::string gate = top_event_->gate();
  boost::to_upper(gate);
  out << "\"" <<  orig_ids_[top_event_id_] << "\" [shape=ellipse, "
      << "fontsize=12, fontcolor=black, fontname=\"times-bold\", "
      << "label=\"" << orig_ids_[top_event_id_] << "\\n"
      << "{ " << gate <<" }\"]\n";
  for (it_inter = inter_events_.begin(); it_inter != inter_events_.end();
       ++it_inter) {
    gate = it_inter->second->gate();
    boost::to_upper(gate);
    out << "\"" <<  orig_ids_[it_inter->first] << "\" [shape=box, "
        << "fontsize=11, fontcolor=blue, "
        << "label=\"" << orig_ids_[it_inter->first] << "\\n"
        << "{ " << gate <<" }\"]\n";
  }
  out.flush();

  std::map<std::string, int>::iterator it;
  for (it = pr_repeat.begin(); it != pr_repeat.end(); ++it) {
    for (int i = 0; i < it->second + 1; ++i) {
      out << "\"" << orig_ids_[it->first] << "_R" << i << "\" [shape=circle, "
          << "height=1, fontsize=10, fixedsize=true, fontcolor=black, "
          << "label=\"" << orig_ids_[it->first] << "\\n["
          << primary_events_[it->first]->type() << "]";
      if (prob_requested_) { out << "\\n" << primary_events_[it->first]->p(); }
      out << "\"]\n";
    }
  }

  out << "}";
  out.flush();
}

void FaultTree::Analyze() {
  // Check if input file with tree instructions has already been read.
  if (input_file_ == "") {
    std::string msg = "Read input file with tree instructions and initialize"
                      " the tree before requesting analysis.";
    throw scram::Error(msg);
  }

  // Container for cut sets with intermediate events.
  std::vector< SupersetPtr > inter_sets;

  // Container for cut sets with primary events only.
  std::vector< std::set<int> > cut_sets;

  FaultTree::AssignIndexes_();

  FaultTree::ExpandSets_(top_event_, inter_sets);

  // An iterator for a vector with sets of ids of events.
  std::vector< std::set<int> >::iterator it_vec;

  // An iterator for a vector with Supersets.
  std::vector< SupersetPtr >::iterator it_sup;

  // Generate cut sets.
  while (!inter_sets.empty()) {
    // Get rightmost set.
    SupersetPtr tmp_set = inter_sets.back();
    // Delete rightmost set.
    inter_sets.pop_back();

    // Discard this tmp set if it is larger than the limit.
    if (tmp_set->NumOfPrimeEvents() > limit_order_) continue;

    if (tmp_set->NumOfInterEvents() == 0) {
      // This is a set with primary events only.
      cut_sets.push_back(tmp_set->primes());
      continue;
    }

    // To hold sets of children.
    std::vector< SupersetPtr > children_sets;

    FaultTree::ExpandSets_(int_to_inter_[tmp_set->PopInter()], children_sets);

    // Attach the original set into this event's sets of children.
    for (it_sup = children_sets.begin(); it_sup != children_sets.end();
         ++it_sup) {
      (*it_sup)->Insert(tmp_set);
      // Add this set to the original inter_sets.
      inter_sets.push_back(*it_sup);
    }
  }

  // At this point cut sets are generated.
  // Now we need to reduce them to minimal cut sets.

  // First, defensive check if cut sets exists for the specified limit order.
  if (cut_sets.empty()) {
    std::stringstream msg;
    msg << "No cut sets for the limit order " <<  limit_order_;
    warnings_ += msg.str();
    analysis_done_ = true;
    return;
  }

  // Choose to convert vector to a set to get rid of any duplications.
  std::set< std::set<int> > unique_cut_sets;
  for (it_vec = cut_sets.begin(); it_vec != cut_sets.end(); ++it_vec) {
    if (it_vec->size() == 1) {
      // Minimal cut set is detected.
      imcs_.insert(*it_vec);
      continue;
    }
    unique_cut_sets.insert(*it_vec);
  }
  // Iterator for unique_cut_sets.
  std::set< std::set<int> >::iterator it_uniq;

  // Iterator for minimal cut sets.
  std::set< std::set<int> >::iterator it_min;

  // Minimal size of sets in uniq_cut_sets.
  int min_size = 2;

  // Min cut sets of the previous size.
  std::set< std::set<int> > mcs_prev_size = imcs_;  // Current size is 1.

  while (!unique_cut_sets.empty()) {
    // Apply rule 4 to reduce unique cut sets.

    std::set< std::set<int> > temp_sets;  // For mcs of a level above.
    std::set< std::set<int> > temp_min_sets;  // For mcs of this level.

    for (it_uniq = unique_cut_sets.begin();
         it_uniq != unique_cut_sets.end(); ++it_uniq) {
         bool include = true;  // Determine to keep or not.

      for (it_min = mcs_prev_size.begin(); it_min != mcs_prev_size.end();
           ++it_min) {
        if (std::includes(it_uniq->begin(), it_uniq->end(),
                          it_min->begin(), it_min->end())) {
          // Non-minimal cut set is detected.
          include = false;
          break;
        }
      }
      // After checking for non-minimal cut sets,
      // all minimum sized cut sets are guaranteed to be minimal.
      if (include) {
        if (it_uniq->size() == min_size) {
          imcs_.insert(*it_uniq);
          temp_min_sets.insert(*it_uniq);
          // Update maximum order of the sets.
          if (min_size > max_order_) max_order_ = min_size;
        } else {
          temp_sets.insert(*it_uniq);
        }
      }
      // Ignore the cut set because include = false.
    }
    unique_cut_sets = temp_sets;
    mcs_prev_size = temp_min_sets;
    min_size++;
  }

  FaultTree::SetsToString_();

  analysis_done_ = true;  // Main analysis enough for reporting is done.

  // Compute probabilities only if requested.
  if (!prob_requested_) return;

  // Maximum number of sums in the series.
  if (nsums_ > imcs_.size()) nsums_ = imcs_.size();

  // Perform Monte Carlo Uncertainty analysis.
  if (analysis_ == "fta-mc") {
    // Generate the equation.
    FaultTree::MProbOr_(imcs_, 1, nsums_);
    // Sample probabilities and generate data.
    FaultTree::MSample_();
    return;
  }

  // Iterate minimal cut sets and find probabilities for each set.
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    // Calculate a probability of a set with AND relationship.
    double p_sub_set = FaultTree::ProbAnd_(*it_min);
    // Update a container with minimal cut sets and probabilities.
    prob_of_min_sets_.insert(std::make_pair(imcs_to_smcs_[*it_min],
                                            p_sub_set));
    ordered_min_sets_.insert(std::make_pair(p_sub_set,
                                            imcs_to_smcs_[*it_min]));
  }

  // Check if a rare event approximation is requested.
  if (rare_event_) {
    warnings_ += "Using the rare event approximation\n";
    bool rare_event_legit = true;
    std::map< std::set<std::string>, double >::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin();
         it_pr != prob_of_min_sets_.end(); ++it_pr) {
      // Check if a probability of a set does not exceed 0.1,
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
    // Exact calculation of probability of cut sets.
    // ---------- Algorithm Improvement Check : Integers -----------------
    // Map primary events to integers.
    p_total_ = FaultTree::ProbOr_(imcs_, nsums_);
    // ------------------------------------------------------------
  }

  // Calculate failure contributions of each primary event.
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator it_prime;
  for (it_prime = primary_events_.begin(); it_prime != primary_events_.end();
       ++it_prime) {
    double contrib = 0;  // Total contribution of this event.
    std::map< std::set<std::string>, double >::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin();
         it_pr != prob_of_min_sets_.end(); ++it_pr) {
      if (it_pr->first.count(it_prime->first)) {
        contrib += it_pr->second;
      }
    }
    imp_of_primaries_.insert(std::make_pair(it_prime->first, contrib));
    ordered_primaries_.insert(std::make_pair(contrib, it_prime->first));
  }
}

void FaultTree::Report(std::string output) {
  // Check if the analysis has been performed before requesting a report.
  if (!analysis_done_) {
    std::string msg = "Perform analysis before calling this report function.";
    throw scram::Error(msg);
  }
  // Check if output to file is requested.
  std::streambuf* buf;
  std::ofstream of;
  if (output != "cli") {
    of.open(output.c_str());
    buf = of.rdbuf();
  } else {
    buf = std::cout.rdbuf();
  }
  std::ostream out(buf);

  // An iterator for a set with ids of events.
  std::set<std::string>::iterator it_set;

  // Iterator for minimal cut sets.
  std::set< std::set<std::string> >::iterator it_min;

  // Iterator for a map with minimal cut sets and their probabilities.
  std::map< std::set<std::string>, double >::iterator it_pr;

  // Print warnings of calculations.
  if (warnings_ != "") {
    out << "\n" << warnings_ << "\n";
  }

  // Print minimal cut sets by their order.
  out << "\n" << "Minimal Cut Sets" << "\n";
  out << "================\n\n";
  out << "Fault Tree: " << input_file_ << "\n";
  out << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << "Analysis algorithm: " << analysis_ << "\n";
  out << "Limit on order of cut sets: " << limit_order_ << "\n";
  out << "Number of Primary Events: " << primary_events_.size() << "\n";
  out << "Minimal Cut Set Maximum Order: " << max_order_ << "\n";
  out.flush();

  int order = 1;  // Order of minimal cut sets.
  std::vector<int> order_numbers;  // Number of sets per order.
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
  out << "  ALL          " << min_cut_sets_.size() << "\n";
  out.flush();

  // Print probabilities of minimal cut sets only if requested.
  if (!prob_requested_) return;

  out << "\n" << "Probability Analysis" << "\n";
  out << "====================\n\n";
  out << "Fault Tree: " << input_file_ << "\n";
  out << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << "Analysis type: " << analysis_ << "\n";
  out << "Limit on series: " << nsums_ << "\n";
  out << "Number of Primary Events: " << primary_events_.size() << "\n";
  out << "Number of Minimal Cut Sets: " << min_cut_sets_.size() << "\n\n";
  out.flush();

  if (analysis_ == "fta-default") {
    out << "Minimal Cut Set Probabilities Sorted by Order:\n";
    out.flush();
    order = 1;  // Order of minimal cut sets.
    std::multimap < double, std::set<std::string> >::reverse_iterator it_or;
    while (order < max_order_ + 1) {
      std::multimap< double, std::set<std::string> > order_sets;
      for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
           ++it_min) {
        if (it_min->size() == order) {
          order_sets.insert(std::make_pair(prob_of_min_sets_[*it_min],
                                           *it_min));
        }
      }
      if (!order_sets.empty()) {
        out << "\nOrder " << order << ":\n";
        int i = 1;
        for (it_or = order_sets.rbegin(); it_or != order_sets.rend(); ++it_or) {
          out << i << ") ";
          out << "{ ";
          for (it_set = it_or->second.begin(); it_set != it_or->second.end();
               ++it_set) {
            out << orig_ids_[*it_set] << " ";
          }
          out << "}    ";
          out << it_or->first << "\n";
          out.flush();
          i++;
        }
      }
      order++;
    }

    out << "\nMinimal Cut Set Probabilities Sorted by Probability:\n\n";
    out.flush();
    int i = 1;
    for (it_or = ordered_min_sets_.rbegin(); it_or != ordered_min_sets_.rend();
         ++it_or) {
      out << i << ") { ";
      for (it_set = it_or->second.begin(); it_set != it_or->second.end();
           ++it_set) {
        out << orig_ids_[*it_set] << " ";
      }
      out << "}    ";
      out << it_or->first << "\n";
      i++;
      out.flush();
    }

    // Print total probability.
    out << "\n" << "=============================\n";
    out <<  "Total Probability: " << p_total_ << "\n";
    out << "=============================\n\n";

    if (p_total_ > 1) out << "WARNING: Total Probability is invalid.\n\n";

    out.flush();

    // Primary event analysis.
    out << "Primary Event Analysis:\n\n";
    out << "Event        Failure Contrib.        Importance\n\n";
    std::multimap < double, std::string >::reverse_iterator it_contr;
    for (it_contr = ordered_primaries_.rbegin();
         it_contr != ordered_primaries_.rend(); ++it_contr) {
      out << orig_ids_[it_contr->second] << "          " << it_contr->first
          << "          " << 100 * it_contr->first / p_total_ << "%\n";
      out.flush();
    }

  } else if (analysis_ == "fta-mc") {
    // Report for Monte Carlo Uncertainty Analysis.
    // Show the terms of the equation.
    // Positive terms.
    out << "\nPositive Terms in the Probability Equation:\n";
    std::vector< std::set<int> >::iterator it_vec;
    std::set<int>::iterator it_set;
    for (it_vec = pos_terms_.begin(); it_vec != pos_terms_.end(); ++it_vec) {
      out << "{ ";
      for (it_set = it_vec->begin(); it_set != it_vec->end(); ++it_set) {
        out << orig_ids_[int_to_prime_[*it_set]->id()] << " ";
      }
      out << "}\n";
      out.flush();
    }
    // Negative terms.
    out << "\nNegative Terms in the Probability Equation:\n";
    for (it_vec = neg_terms_.begin(); it_vec != neg_terms_.end(); ++it_vec) {
      out << "{ ";
      for (it_set = it_vec->begin(); it_set != it_vec->end(); ++it_set) {
        out << orig_ids_[int_to_prime_[*it_set]->id()] << " ";
      }
      out << "}\n";
      out.flush();
    }
  }
}

bool FaultTree::GetArgs_(std::vector<std::string>& args, std::string& line,
                         std::string& orig_line) {
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

void FaultTree::InterpretArgs_(int nline, std::stringstream& msg,
                               std::vector<std::string>& args,
                               std::string& orig_line,
                               std::string tr_parent,
                               std::string tr_id,
                               std::string suffix) {
  assert(nline > 0);  // Sanity checks.
  assert(args.size() != 0);  // Empty input args shouldn't be passed.

  switch (args.size()) {
    case 1: {
      if (args[0] == "{") {
        // Check if this is a new start of a block.
        if (block_started_) {
          msg << "Line " << nline << " : " << "Found second '{' before"
              << " finishing the current block.";
          throw scram::ValidationError(msg.str());
        }
        // New block successfully started.
        block_started_ = true;

      } else if (args[0] == "}") {
        // Check if this is an end of a block.
        if (!block_started_) {
          msg << "Line " << nline << " : " << " Found '}' before starting"
              << " a new block.";
          throw scram::ValidationError(msg.str());
        }
        // End of the block detected.

        // Check if all needed arguments for an event are received..
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
        }

        // Special case for sub-tree analysis without a main file.
        if (is_main_ && type_ == "transferout") {
          if (sub_tree_analysis_) {
            msg << "Line " << nline << " : Found a second TransferOut in a"
                << " in a sub-tree analysis file.";
            throw scram::ValidationError(msg.str());
          }

          std::string tr_id =
              input_file_.substr(input_file_.find_last_of("/") +
                                 1, std::string::npos);
          if (top_event_id_ != "") {
            msg << "Line " << nline << " : TransferOut must be a separate tree"
                << " in a separate file.";
            throw scram::ValidationError(msg.str());
          } else if (parent_ != "any") {
            msg << "Line " << nline << " : Parent of TransferOut must be "
                << "'Any'.";
            throw scram::ValidationError(msg.str());
          } else if (id_ != tr_id) {
            msg << "Line " << nline << " : Id of the first node in "
                << "TransferOut must match the name of the file.";
            throw scram::ValidationError(msg.str());
          }
          sub_tree_analysis_ = true;
          transfer_first_inter_ = false;
          // Refresh values.
          parent_ = "";
          id_ = "";
          type_ = "";
          block_started_ = false;
          break;
        }

        if (is_main_ && sub_tree_analysis_) {
          std::string tr_id =
              input_file_.substr(input_file_.find_last_of("/") +
                                 1, std::string::npos);
          if (!transfer_first_inter_) {
            // Check the second node's validity.
            if (parent_ != tr_id) {
              msg << "Line " << nline << " : Parent of the first intermediate "
                  << "event must be the name of the file.";
              throw scram::ValidationError(msg.str());
            } else if (types_.count(type_)) {
              msg << "Line " << nline << " : Type of the first intermediate "
                  << "event in the transfer tree cannot be a primary event.";
              throw scram::ValidationError(msg.str());
            }
            parent_ = "none";  // Making this event a root.
            // Add a node with the gathered information.
            FaultTree::AddNode_(parent_, id_, type_);  // No error expected.
            transfer_first_inter_ = true;
            // Refresh values.
            parent_ = "";
            id_ = "";
            type_ = "";
            block_started_ = false;
            break;
          } else {
            // Defensive check if there are other events having the name of
            // the current transfer file as their parent.
            if (parent_ == tr_id) {
              msg << "Line " << nline << " : Found the second event with "
                  << "a parent as a starting node. Only one intermediate event"
                  << " can be linked to TransferOut of the transfer tree.";
              throw scram::ValidationError(msg.str());
            }
          }
        }

        if (!is_main_) {
          // Inidication that this is a transfer subtree.
          // Check for correct transfer initialization.
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
            // Refresh values.
            parent_ = "";
            id_ = "";
            type_ = "";
            block_started_ = false;
            break;
          }

          // Check if this is the main intermediate event in the transfer tree.
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
            // Re-assign parent of the first intermediate event in this
            // sub-tree to the parent of the transfer in order to link
            // trees.
            parent_ = tr_parent;
            transfer_first_inter_ = true;
          } else {
            // Defensive check if there are other events having the name of
            // the current transfer file as their parent.
            if (parent_ == tr_id) {
              msg << "Line " << nline << " : Found the second event with "
                  << "a parent as a starting node. Only one intermediate event"
                  << " can be linked to TransferOut of the transfer tree.";
              throw scram::ValidationError(msg.str());
            }

            // Attach specific suffix for the parents of events
            // in the sub-tree.
            parent_ += suffix;
          }

          // Defensive check if there is another TransferOut in the same file.
          if (type_ == "transferout") {
            msg << "Line " << nline << " : Found the second TransferOut in "
                << "the same transfer sub-tree definition.";
            throw scram::ValidationError(msg.str());
          }

          // Attach specific suffix for the intermediate events.
          if (gates_.count(type_)) {
            id_ += suffix;
          }
        }  // Transfer tree related name manipulations are done.

        try {
          // Add a node with the gathered information.
          FaultTree::AddNode_(parent_, id_, type_);
        } catch (scram::ValidationError& err) {
          msg << "Line " << nline << " : " << err.msg();
          throw scram::ValidationError(msg.str());
        }

        // Refresh values.
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
      if (!block_started_) {
        msg << "Line " << nline << " : " << "Missing opening bracket {";
        throw scram::ValidationError(msg.str());
      }

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
        if (!gates_.count(type_) && !types_.count(type_)
            && (type_ != "transferin") && (type_ != "transferout")) {
          boost::to_upper(type_);
          msg << "Line " << nline << " : " << "Invalid input arguments."
              << " Do not support this '" << type_
              << "' gate/event type.";
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

void FaultTree::AddNode_(std::string parent, std::string id,
                         std::string type) {
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
      top_event_ = TopEventPtr(new scram::TopEvent(top_event_id_));

      // Top event cannot be primary.
      if (!gates_.count(type)) {
        std::stringstream msg;
        msg << "Invalid input arguments. " << orig_ids_[top_event_id_]
            << " top event type is not defined correctly. It must be a gate.";
        throw scram::ValidationError(msg.str());
      }

      top_event_->gate(type);

    } else {
      // Another top event is detected.
      std::stringstream msg;
      msg << "Invalid input arguments. The input cannot contain more than"
          << " one top event. " << orig_ids_[id] << " is redundant.";
      throw scram::ValidationError(msg.str());
    }

  } else if (types_.count(type)) {
    // This must be a primary event.
    PrimaryEventPtr p_event(new PrimaryEvent(id, type));
    if (parent == top_event_id_) {
      p_event->AddParent(top_event_);
      top_event_->AddChild(p_event);
      primary_events_.insert(std::make_pair(id, p_event));

    } else if (inter_events_.count(parent)) {
      p_event->AddParent(inter_events_[parent]);
      inter_events_[parent]->AddChild(p_event);
      primary_events_.insert(std::make_pair(id, p_event));

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

    InterEventPtr i_event(new scram::InterEvent(id));

    if (parent == top_event_id_) {
      i_event->parent(top_event_);
      top_event_->AddChild(i_event);
      inter_events_.insert(std::make_pair(id, i_event));

    } else if (inter_events_.count(parent)) {
      i_event->parent(inter_events_[parent]);
      inter_events_[parent]->AddChild(i_event);
      inter_events_.insert(std::make_pair(id, i_event));

    } else {
      // Parent is undefined.
      std::stringstream msg;
      msg << "Invalid input arguments. Parent of " << orig_ids_[id]
          << " intermediate event is not defined.";
      throw scram::ValidationError(msg.str());
    }

    i_event -> gate(type);
  }
}

void FaultTree::AddProb_(std::string id, double p) {
  // Check if the primary event is in this tree.
  if (!primary_events_.count(id)) {
    return;  // Ignore non-existent assignment.
  }

  primary_events_[id]->p(p);
}

void FaultTree::AddProb_(std::string id, double p, double time) {
  // Check if the primary event is in this tree.
  if (!primary_events_.count(id)) {
    return;  // Ignore non-existent assignment.
  }

  primary_events_[id]->p(p, time);
}

void FaultTree::IncludeTransfers_() {
  is_main_ = false;
  while (!transfers_.empty()) {
    std::pair<std::string, std::string> tr_node = transfers_.front();
    transfers_.pop();

    // Parent of this transfer symbol.
    std::string tr_parent = tr_node.first;
    // Id of this transfer symbol, which is the name of the file.
    std::string tr_id = tr_node.second;
    // Increase the number of calls of this sub-tree.
    trans_calls_[tr_id] += 1;
    // An attachment to intermediate event names in order to avoid clashes.
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

    // Register that a sub-tree file is under operation.
    current_file_ = tr_id;

    std::string line = "";  // Case insensitive input.
    std::string orig_line = "";  // Line with capitalizations preserved.
    std::vector<std::string> args;  // To hold args after splitting the line.

    // Error messages.
    std::stringstream msg;
    // File specific indication for errors.
    msg << "In " << tr_id << ", ";

    // Indicate if TransferOut is initiated correctly.
    transfer_correct_ = false;

    // Indication of the first intermediate event of the transfer.
    transfer_first_inter_ = false;

    for (int nline = 1; getline(ifile, line); ++nline) {
      if (!FaultTree::GetArgs_(args, line, orig_line)) continue;

      FaultTree::InterpretArgs_(nline, msg,  args, orig_line, tr_parent,
                                tr_id, suffix);
    }
  }
}

void FaultTree::GraphNode_(TopEventPtr t,
                           std::map<std::string, int>& pr_repeat,
                           std::ofstream& out) {
  // Populate intermediate and primary events of the input inter event.
  std::map<std::string, boost::shared_ptr<scram::Event> >
      events_children = t->children();
  std::map<std::string, boost::shared_ptr<scram::Event> >::iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    // Deal with repeated primary events.
    if (primary_events_.count(it_child->first)) {
      if (pr_repeat.count(it_child->first)) {
        int rep = pr_repeat[it_child->first];
        rep++;
        pr_repeat.erase(it_child->first);
        pr_repeat.insert(std::make_pair(it_child->first, rep));
      } else if (!inter_events_.count(it_child->first)) {
        pr_repeat.insert(std::make_pair(it_child->first, 0));
      }
      out << "\"" <<  orig_ids_[t->id()] << "\" -> "
          << "\"" <<orig_ids_[it_child->first] <<"_R"
          << pr_repeat[it_child->first] << "\";\n";
    } else {
      out << "\"" << orig_ids_[t->id()] << "\" -> "
          << "\"" << orig_ids_[it_child->first] << "\";\n";
    }
  }
}

void FaultTree::ExpandSets_(const TopEventPtr& t,
                            std::vector< SupersetPtr >& sets) {
  // Populate intermediate and primary events of the top.
  std::map<std::string, boost::shared_ptr<scram::Event> >
      events_children = t->children();

  // Iterator for children of top and intermediate events.
  std::map<std::string, boost::shared_ptr<scram::Event> >::iterator it_child;

  // Type dependent logic.
  if (t->gate() == "or") {
    for (it_child = events_children.begin();
         it_child != events_children.end(); ++it_child) {
      SupersetPtr tmp_set_c(new scram::Superset());
      if (inter_events_.count(it_child->first)) {
        tmp_set_c->AddInter(inter_to_int_[it_child->first]);
      } else {
        tmp_set_c->AddPrimary(prime_to_int_[it_child->first]);
      }
      sets.push_back(tmp_set_c);
    }
  } else if (t->gate() == "and") {
    SupersetPtr tmp_set_c(new scram::Superset());
    for (it_child = events_children.begin();
         it_child != events_children.end(); ++it_child) {
      if (inter_events_.count(it_child->first)) {
        tmp_set_c->AddInter(inter_to_int_[it_child->first]);
      } else {
        tmp_set_c->AddPrimary(prime_to_int_[it_child->first]);
      }
    }
    sets.push_back(tmp_set_c);
  } else {
    std::string msg = "No algorithm defined for" + t->gate();
    throw scram::ValueError(msg);
  }
}

std::string FaultTree::CheckAllGates_() {
  // Handle the special case when only one node TransferIn tree is graphed.
  if (graph_only_ && top_event_id_ == "") return "";

  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.

  // Check the top event.
  msg << FaultTree::CheckGate_(top_event_);

  // Check the intermediate events.
  boost::unordered_map<std::string, InterEventPtr>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    msg << FaultTree::CheckGate_(it->second);
  }

  return msg.str();
}

std::string FaultTree::CheckGate_(TopEventPtr event) {
  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.
  try {
    std::string gate = event->gate();
    // This line throws error if there are no children.
    int size = event->children().size();
    // Add transfer gates if needed for graphing.
    size += transfer_map_.count(event->id());

    // Gate dependent logic.
    if (gate == "and" || gate == "or") {
      if (size < 2) {
        boost::to_upper(gate);
        msg << orig_ids_[event->id()] << " : " << gate
            << " gate must have 2 or more "
            << "children.\n";
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

// -------------------- Algorithm for Cut Sets and Probabilities -----------
double FaultTree::ProbOr_(std::set< std::set<int> >& min_cut_sets, int nsums) {
  // Recursive implementation.
  if (min_cut_sets.empty()) {
    throw scram::ValueError("Do not pass empty set to prob_or_ function.");
  }

  if (nsums == 0) {
    return 0;
  }

  // Base case.
  if (min_cut_sets.size() == 1) {
    // Get only element in this set.
    return FaultTree::ProbAnd_(*min_cut_sets.begin());
  }

  // Get one element.
  std::set< std::set<int> >::iterator it = min_cut_sets.begin();
  std::set<int> element_one = *it;

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets.erase(it);
  std::set< std::set<int> > combo_sets;
  FaultTree::CombineElAndSet_(element_one, min_cut_sets, combo_sets);

  return FaultTree::ProbAnd_(element_one) +
         FaultTree::ProbOr_(min_cut_sets, nsums) -
         FaultTree::ProbOr_(combo_sets, nsums - 1);
}

double FaultTree::ProbAnd_(const std::set<int>& min_cut_set) {
  // Test just in case the min cut set is empty.
  if (min_cut_set.empty()) {
    throw scram::ValueError("The set is empty for probability calculations.");
  }

  double p_sub_set = 1;  // 1 is for multiplication.
  std::set<int>::iterator it_set;
  for (it_set = min_cut_set.begin(); it_set != min_cut_set.end(); ++it_set) {
    p_sub_set *= iprobs_[*it_set];
  }
  return p_sub_set;
}

void FaultTree::CombineElAndSet_(const std::set<int>& el,
                                 const std::set< std::set<int> >& set,
                                 std::set< std::set<int> >& combo_set) {
  std::set<int> member_set;
  std::set< std::set<int> >::iterator it_set;
  for (it_set = set.begin(); it_set != set.end(); ++it_set) {
    member_set = *it_set;
    member_set.insert(el.begin(), el.end());
    combo_set.insert(member_set);
  }
}

void FaultTree::AssignIndexes_() {
  // Assign an index to each primary event, and populate relevant
  // databases.
  int j = 0;
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator itp;
  for (itp = primary_events_.begin(); itp != primary_events_.end(); ++itp) {
    int_to_prime_.push_back(itp->second);
    prime_to_int_.insert(std::make_pair(itp->second->id(), j));
    if (prob_requested_) iprobs_.push_back(itp->second->p());
    ++j;
  }

  // Assign an index to each top and intermediate event and populate
  // relevant databases.
  boost::unordered_map<std::string, InterEventPtr>::iterator iti;
  for (iti = inter_events_.begin(); iti != inter_events_.end(); ++iti) {
    int_to_inter_.insert(std::make_pair(j, iti->second));
    inter_to_int_.insert(std::make_pair(iti->second->id(), j));
    ++j;
  }
}

void FaultTree::SetsToString_() {
  std::set< std::set<int> >::iterator it_min;
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      pr_set.insert(int_to_prime_[*it_set]->id());
    }
    imcs_to_smcs_.insert(std::make_pair(*it_min, pr_set));
    min_cut_sets_.insert(pr_set);
  }
}

// ----------------------------------------------------------------------
// ----- Algorithm for Total Equation for Monte Carlo Simulation --------
// Generation of the representation of the original equation.
void FaultTree::MProbOr_(std::set< std::set<int> >& min_cut_sets,
                         int sign, int nsums) {
  // Recursive implementation.
  if (min_cut_sets.empty()) {
    throw scram::ValueError("Do not pass empty set to prob_or_ function.");
  }

  if (nsums == 0) {
    return;
  }

  // Get one element.
  std::set< std::set<int> >::iterator it = min_cut_sets.begin();
  std::set<int> element_one = *it;

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets.erase(it);

  // Put this element into the equation.
  if ((sign % 2) == 1) {
    // This is a positive member.
    pos_terms_.push_back(element_one);
  } else {
    // This must be a negative member.
    neg_terms_.push_back(element_one);
  }

  // Base case.
  if (min_cut_sets.empty()) return;

  std::set< std::set<int> > combo_sets;
  FaultTree::MCombineElAndSet_(element_one, min_cut_sets, combo_sets);
  FaultTree::MProbOr_(min_cut_sets, sign, nsums);
  FaultTree::MProbOr_(combo_sets, sign + 1, nsums - 1);
  return;
}

void FaultTree::MCombineElAndSet_(const std::set<int>& el,
                                  const std::set< std::set<int> >& set,
                                  std::set< std::set<int> >& combo_set) {
  FaultTree::CombineElAndSet_(el, set, combo_set);  // The same function.
}

void FaultTree::MSample_() {}
// ----------------------------------------------------------------------

}  // namespace scram
