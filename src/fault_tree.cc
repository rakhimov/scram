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

FaultTree::FaultTree(std::string analysis, bool graph_only,
                     std::string approx, int limit_order, int nsums)
    : graph_only_(graph_only),
      warnings_(""),
      top_event_id_(""),
      top_event_index_(-1),
      top_detected_(false),
      is_main_(true),
      sub_tree_analysis_(false),
      input_file_(""),
      prob_requested_(false),
      analysis_done_(false),
      max_order_(1),
      p_total_(0),
      exp_time_(0),
      mcs_time_(0),
      p_time_(0),
      parent_(""),
      id_(""),
      type_(""),
      vote_number_(-1),
      block_started_(false),
      transfer_correct_(false),
      transfer_first_inter_(false) {
  // Check for valid analysis type.
  if (analysis != "default" && analysis != "mc") {
    std::string msg = "The analysis type is not recognized.";
    throw scram::ValueError(msg);
  }
  analysis_ = analysis;

  // Check for right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw scram::ValueError(msg);
  }
  limit_order_ = limit_order;

  // Check for right limit order.
  if (nsums < 1) {
    std::string msg = "The number of sums in the probability calculation "
                      "cannot be less than one";
    throw scram::ValueError(msg);
  }
  nsums_ = nsums;

  // Check the right approximation for probability calculations.
  if (approx != "no" && approx != "rare" && approx != "mcub") {
    std::string msg = "The probability approximation is not recognized.";
    throw scram::ValueError(msg);
  }
  approx_ = approx;

  // Add valid gates.
  gates_.insert("and");
  gates_.insert("or");
  gates_.insert("not");
  gates_.insert("nor");
  gates_.insert("nand");
  gates_.insert("xor");
  gates_.insert("null");
  gates_.insert("inhibit");
  gates_.insert("vote");

  // Add valid primary event types.
  types_.insert("basic");
  types_.insert("undeveloped");
  types_.insert("house");
  types_.insert("conditional");

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
    if (!FaultTree::GetArgs_(line, orig_line, args)) continue;

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
    if (!FaultTree::GetArgs_(line, orig_line, args)) continue;

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
    throw scram::IOError(output_path +  " : Cannot write the graphing file.");
  }

  // Check for the special case when only one node TransferIn tree is graphed.
  if (top_event_id_ == "") {
    std::string msg = "Cannot graph one node TransferIn tree.";
    throw scram::ValidationError(msg);
  }

  boost::to_upper(graph_name);
  out << "digraph " << graph_name << " {\n";
  // Special case for sub-tree only graphing.
  if (sub_tree_analysis_) {
    std::string tr_id = input_file_.substr(input_file_.find_last_of("/") + 1,
                                           std::string::npos);
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
  std::map<std::string, std::string> gate_colors;
  gate_colors.insert(std::make_pair("or", "blue"));
  gate_colors.insert(std::make_pair("and", "green"));
  gate_colors.insert(std::make_pair("not", "red"));
  gate_colors.insert(std::make_pair("xor", "brown"));
  gate_colors.insert(std::make_pair("inhibit", "yellow"));
  gate_colors.insert(std::make_pair("vote", "cyan"));
  gate_colors.insert(std::make_pair("null", "gray"));
  gate_colors.insert(std::make_pair("nor", "magenta"));
  gate_colors.insert(std::make_pair("nand", "orange"));
  std::string gate = top_event_->gate();
  boost::to_upper(gate);
  out << "\"" <<  orig_ids_[top_event_id_] << "\" [shape=ellipse, "
      << "fontsize=12, fontcolor=black, fontname=\"times-bold\", "
      << "color=" << gate_colors[top_event_->gate()] << ", "
      << "label=\"" << orig_ids_[top_event_id_] << "\\n"
      << "{ " << gate;
  if (gate == "VOTE") {
    out << " " << top_event_->vote_number() << "/"
        << top_event_->children().size();
  }
  out << " }\"]\n";
  for (it_inter = inter_events_.begin(); it_inter != inter_events_.end();
       ++it_inter) {
    gate = it_inter->second->gate();
    boost::to_upper(gate);
    out << "\"" <<  orig_ids_[it_inter->first] << "\" [shape=box, "
        << "fontsize=11, fontcolor=black, "
        << "color=" << gate_colors[it_inter->second->gate()] << ", "
        << "label=\"" << orig_ids_[it_inter->first] << "\\n"
        << "{ " << gate;
    if (gate == "VOTE") {
      out << " " << it_inter->second->vote_number() << "/"
          << it_inter->second->children().size();
    }
    out << " }\"]\n";
  }
  out.flush();

  std::map<std::string, std::string> event_colors;
  event_colors.insert(std::make_pair("basic", "black"));
  event_colors.insert(std::make_pair("undeveloped", "blue"));
  event_colors.insert(std::make_pair("house", "green"));
  event_colors.insert(std::make_pair("conditional", "red"));
  std::map<std::string, int>::iterator it;
  for (it = pr_repeat.begin(); it != pr_repeat.end(); ++it) {
    for (int i = 0; i < it->second + 1; ++i) {
      out << "\"" << orig_ids_[it->first] << "_R" << i << "\" [shape=circle, "
          << "height=1, fontsize=10, fixedsize=true, "
          << "fontcolor=" << event_colors[primary_events_[it->first]->type()]
          << ", " << "label=\"" << orig_ids_[it->first] << "\\n["
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

  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();
  // End of Timing Initialization

  // Container for cut sets with intermediate events.
  std::vector< SupersetPtr > inter_sets;

  // Container for cut sets with primary events only.
  std::vector< std::set<int> > cut_sets;

  FaultTree::AssignIndices_();

  FaultTree::ExpandSets_(top_event_index_, inter_sets);

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

    FaultTree::ExpandSets_(tmp_set->PopInter(), children_sets);

    // Attach the original set into this event's sets of children.
    for (it_sup = children_sets.begin(); it_sup != children_sets.end();
         ++it_sup) {
      // Add this set to the original inter_sets.
      if ((*it_sup)->Insert(tmp_set)) inter_sets.push_back(*it_sup);
    }
  }

  // Duration of the expansion.
  exp_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);

  // At this point cut sets are generated.
  // Now we need to reduce them to minimal cut sets.

  // First, defensive check if cut sets exist for the specified limit order.
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

  FaultTree::FindMCS_(unique_cut_sets, imcs_, 2);
  // Duration of MCS generation.
  mcs_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
  FaultTree::SetsToString_();  // MCS with event ids.

  analysis_done_ = true;  // Main analysis enough for reporting is done.

  // Compute probabilities only if requested.
  if (!prob_requested_) return;

  // Maximum number of sums in the series.
  if (nsums_ > imcs_.size()) nsums_ = imcs_.size();

  // Perform Monte Carlo Uncertainty analysis.
  if (analysis_ == "mc") {
    // Generate the equation.
    FaultTree::MProbOr_(imcs_, 1, nsums_);
    // Sample probabilities and generate data.
    FaultTree::MSample_();
    return;
  }

  // Iterator for minimal cut sets.
  std::set< std::set<int> >::iterator it_min;

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

  // Check if the rare event approximation is requested.
  if (approx_ == "rare") {
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

  } else if (approx_ == "mcub") {
    warnings_ += "Using the MCUB approximation\n";
    double m = 1;
    std::map< std::set<std::string>, double >::iterator it;
    for (it = prob_of_min_sets_.begin(); it != prob_of_min_sets_.end();
         ++it) {
      m *= 1 - it->second;
    }
    p_total_ = 1 - m;

  } else {  // No approximation technique is assumed.
    // Exact calculation of probability of cut sets.
    p_total_ = FaultTree::ProbOr_(imcs_, nsums_);
  }

  // Calculate failure contributions of each primary event.
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator it_prime;
  for (it_prime = primary_events_.begin(); it_prime != primary_events_.end();
       ++it_prime) {
    double contrib_pos = 0;  // Total positive contribution of this event.
    double contrib_neg = 0;  // Negative event contribution.
    std::map< std::set<std::string>, double >::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin();
         it_pr != prob_of_min_sets_.end(); ++it_pr) {
      if (it_pr->first.count(it_prime->first)) {
        contrib_pos += it_pr->second;
      } else if (it_pr->first.count("not " + it_prime->first)) {
        contrib_neg += it_pr->second;
      }
    }
    imp_of_primaries_.insert(std::make_pair(it_prime->first, contrib_pos));
    ordered_primaries_.insert(std::make_pair(contrib_pos, it_prime->first));
    if (contrib_neg > 0) {
      imp_of_primaries_.insert(std::make_pair("not " + it_prime->first,
                                              contrib_neg));
      ordered_primaries_.insert(std::make_pair(contrib_neg,
                                               "not " + it_prime->first));
    }
  }
  // Duration of probability related operations.
  p_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);}

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

  // Convert MCS into representative strings.
  std::map< std::set<std::string>, std::string> represent;
  std::map< std::set<std::string>, std::vector<std::string> > lines;
  for (it_min = min_cut_sets_.begin(); it_min != min_cut_sets_.end();
       ++it_min) {
    std::stringstream rep;
    rep << "{ ";
    std::string line = "{ ";
    std::vector<std::string> vec_line;
    int j = 1;
    int size = it_min->size();
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      std::vector<std::string> names;
      boost::split(names, *it_set, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() < 3);
      assert(names.size() > 0);
      std::string name = "";
      if (names.size() == 1) {
        name = orig_ids_[names[0]];
      } else if (names.size() == 2) {
        name = "NOT " + orig_ids_[names[1]];
      }
      rep << name;

      if (line.length() + name.length() + 2 > 60) {
        vec_line.push_back(line);
        line = name;
      } else {
        line += name;
      }

      if (j < size) {
        rep << ", ";
        line += ", ";
      } else {
        rep << " ";
        line += " ";
      }
      ++j;
    }
    rep << "}";
    line += "}";
    vec_line.push_back(line);
    represent.insert(std::make_pair(*it_min, rep.str()));
    lines.insert(std::make_pair(*it_min, vec_line));
  }

  // Print warnings of calculations.
  if (warnings_ != "") {
    out << "\n" << warnings_ << "\n";
  }

  // Print minimal cut sets by their order.
  out << "\n" << "Minimal Cut Sets" << "\n";
  out << "================\n\n";
  out << std::setw(40) << std::left << "Fault Tree: " << input_file_ << "\n";
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Analysis algorithm: " << analysis_ << "\n";
  out << std::setw(40) << "Limit on order of cut sets: " << limit_order_ << "\n";
  out << std::setw(40) << "Number of Primary Events: " << primary_events_.size() << "\n";
  out << std::setw(40) << "Minimal Cut Set Maximum Order: " << max_order_ << "\n";
  out << std::setw(40) << "Gate Expansion Time: " << std::setprecision(5)
      << exp_time_ << "s\n";
  out << std::setw(40) << "MCS Generation Time: " << std::setprecision(5)
      << mcs_time_ - exp_time_ << "s\n";
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
        std::stringstream number;
        number << i << ") ";
        out << std::left;
        std::vector<std::string>::iterator it;
        int j = 0;
        for (it = lines[*it_min].begin(); it != lines[*it_min].end(); ++it) {
          if (j == 0) {
            out << number.str() <<  *it << "\n";
          } else {
            out << "  " << std::setw(number.str().length()) << " "
                << *it << "\n";
          }
          ++j;
        }
        out.flush();
        i++;
      }
    }
    order++;
  }

  out << "\nQualitative Importance Analysis:" << "\n";
  out << "--------------------------------\n";
  out << std::left;
  out << std::setw(20) << "Order" << "Number\n";
  out << std::setw(20) << "-----" << "------\n";
  for (int i = 1; i < max_order_ + 1; ++i) {
    out << "  " << std::setw(18) << i << order_numbers[i-1] << "\n";
  }
  out << "  " << std::setw(18) << "ALL" << min_cut_sets_.size() << "\n";
  out.flush();

  // Print probabilities of minimal cut sets only if requested.
  if (!prob_requested_) return;

  out << "\n" << "Probability Analysis" << "\n";
  out << "====================\n\n";
  out << std::setw(40) << std::left << "Fault Tree: " << input_file_ << "\n";
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Analysis type:" << analysis_ << "\n";
  out << std::setw(40) << "Limit on series: " << nsums_ << "\n";
  out << std::setw(40) << "Number of Primary Events: "
      << primary_events_.size() << "\n";
  out << std::setw(40) << "Number of Minimal Cut Sets: "
      << min_cut_sets_.size() << "\n";
  out << std::setw(40) << "Probability Operations Time: " << std::setprecision(5)
      << p_time_ - mcs_time_ << "s\n\n";
  out.flush();

  if (analysis_ == "default") {
    out << "Minimal Cut Set Probabilities Sorted by Order:\n";
    out << "----------------------------------------------\n";
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
          std::stringstream number;
          number << i << ") ";
          out << std::left;
          std::vector<std::string>::iterator it;
          int j = 0;
          for (it = lines[it_or->second].begin();
               it != lines[it_or->second].end(); ++it) {
            if (j == 0) {
              out << number.str() << std::setw(70 - number.str().length())
                  << *it << std::setprecision(7) << it_or->first << "\n";
            } else {
              out << "  " << std::setw(number.str().length()) << " "
                  << *it << "\n";
            }
            ++j;
          }
          out.flush();
          i++;
        }
      }
      order++;
    }

    out << "\nMinimal Cut Set Probabilities Sorted by Probability:\n";
    out << "----------------------------------------------------\n";
    out.flush();
    int i = 1;
    for (it_or = ordered_min_sets_.rbegin(); it_or != ordered_min_sets_.rend();
         ++it_or) {
      std::stringstream number;
      number << i << ") ";
      out << std::left;
      std::vector<std::string>::iterator it;
      int j = 0;
      for (it = lines[it_or->second].begin();
           it != lines[it_or->second].end(); ++it) {
        if (j == 0) {
          out << number.str() << std::setw(70 - number.str().length())
              << *it << std::setprecision(7) << it_or->first << "\n";
        } else {
          out << "  " << std::setw(number.str().length()) << " "
              << *it << "\n";
        }
        ++j;
      }
      i++;
      out.flush();
    }

    // Print total probability.
    out << "\n" << "================================\n";
    out <<  "Total Probability: " << std::setprecision(7) << p_total_ << "\n";
    out << "================================\n\n";

    if (p_total_ > 1) out << "WARNING: Total Probability is invalid.\n\n";

    out.flush();

    // Primary event analysis.
    out << "Primary Event Analysis:\n";
    out << "-----------------------\n";
    out << std::left;
    out << std::setw(20) << "Event" << std::setw(20) << "Failure Contrib."
        << "Importance\n\n";
    std::multimap < double, std::string >::reverse_iterator it_contr;
    for (it_contr = ordered_primaries_.rbegin();
         it_contr != ordered_primaries_.rend(); ++it_contr) {
      out << std::left;
      std::vector<std::string> names;
      boost::split(names, it_contr->second, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() < 3);
      assert(names.size() > 0);
      if (names.size() == 1) {
        out << std::setw(20) << orig_ids_[names[0]] << std::setw(20)
            << it_contr->first << 100 * it_contr->first / p_total_ << "%\n";

      } else if (names.size() == 2) {
        out << "NOT " << std::setw(16) << orig_ids_[names[1]] << std::setw(20)
            << it_contr->first << 100 * it_contr->first / p_total_ << "%\n";
      }
      out.flush();
    }

  } else if (analysis_ == "mc") {
    // Report for Monte Carlo Uncertainty Analysis.
    // Show the terms of the equation.
    // Positive terms.
    out << "\nPositive Terms in the Probability Equation:\n";
    out << "--------------------------------------------\n";
    std::vector< std::set<int> >::iterator it_vec;
    std::set<int>::iterator it_set;
    for (it_vec = pos_terms_.begin(); it_vec != pos_terms_.end(); ++it_vec) {
      out << "{ ";
      int j = 1;
      int size = it_vec->size();
      for (it_set = it_vec->begin(); it_set != it_vec->end(); ++it_set) {
        if (*it_set > 0) {
          out << orig_ids_[int_to_prime_[*it_set]->id()];
        } else {
          out << "NOT " << orig_ids_[int_to_prime_[std::abs(*it_set)]->id()];
        }
        if (j < size) {
          out << ", ";
        } else {
          out << " ";
        }
        ++j;
      }
      out << "}\n";
      out.flush();
    }
    // Negative terms.
    out << "\nNegative Terms in the Probability Equation:\n";
    out << "-------------------------------------------\n";
    for (it_vec = neg_terms_.begin(); it_vec != neg_terms_.end(); ++it_vec) {
      out << "{ ";
      int j = 1;
      int size = it_vec->size();
      for (it_set = it_vec->begin(); it_set != it_vec->end(); ++it_set) {
        if (*it_set > 0) {
          out << orig_ids_[int_to_prime_[*it_set]->id()];
        } else {
          out << "NOT " << orig_ids_[int_to_prime_[std::abs(*it_set)]->id()];
        }
        if (j < size) {
          out << ", ";
        } else {
          out << " ";
        }
        ++j;
      }
      out << "}\n";
      out.flush();
    }
  }
}

bool FaultTree::GetArgs_(std::string& line, std::string& orig_line,
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
          FaultTree::AddNode_(parent_, id_, type_, vote_number_);
        } catch (scram::ValidationError& err) {
          msg << "Line " << nline << " : " << err.msg();
          throw scram::ValidationError(msg.str());
        }

        // Refresh values.
        parent_ = "";
        id_ = "";
        type_ = "";
        vote_number_ = -1;
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
      } else if (args[0] == "votenumber" && vote_number_ == -1) {
        try {
          vote_number_ = boost::lexical_cast<int>(args[1]);
        } catch (boost::bad_lexical_cast err) {
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

void FaultTree::AddNode_(std::string parent, std::string id,
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
      top_event_ = TopEventPtr(new scram::TopEvent(top_event_id_));

      // Top event cannot be primary.
      if (!gates_.count(type)) {
        std::stringstream msg;
        msg << "Invalid input arguments. " << orig_ids_[top_event_id_]
            << " top event type is not defined correctly. It must be a gate.";
        throw scram::ValidationError(msg.str());
      }

      top_event_->gate(type);
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
      if (type == "conditional" && top_event_->gate() != "inhibit") {
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
      if (type == "conditional" && inter_events_[parent]->gate() != "inhibit") {
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
    if (type == "vote") i_event->vote_number(vote_number);
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
      throw scram::IOError(msg);
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
      if (!FaultTree::GetArgs_(line, orig_line, args)) continue;

      FaultTree::InterpretArgs_(nline, msg,  args, orig_line, tr_parent,
                                tr_id, suffix);
    }
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

std::string FaultTree::CheckGate_(const TopEventPtr& event) {
  std::stringstream msg;
  msg << "";  // An empty default message is the indicator of no problems.
  try {
    std::string gate = event->gate();
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

void FaultTree::GraphNode_(TopEventPtr t,
                           std::map<std::string, int>& pr_repeat,
                           std::ofstream& out) {
  // Populate intermediate and primary events of the input inter event.
  std::map<std::string, EventPtr> events_children = t->children();
  std::map<std::string, EventPtr>::iterator it_child;
  for (it_child = events_children.begin(); it_child != events_children.end();
       ++it_child) {
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

void FaultTree::ExpandSets_(int inter_index,
                            std::vector< SupersetPtr >& sets) {
  // Populate intermediate and primary events of the top.
  const std::map<std::string, EventPtr>* children =
      &int_to_inter_[std::abs(inter_index)]->children();

  std::string gate = int_to_inter_[std::abs(inter_index)]->gate();

  // Iterator for children of top and intermediate events.
  std::map<std::string, EventPtr>::const_iterator it_children;
  std::vector<int> events_children;
  std::vector<int>::iterator it_child;

  for (it_children = children->begin();
       it_children != children->end(); ++it_children) {
    if (inter_events_.count(it_children->first)) {
      events_children.push_back(inter_to_int_[it_children->first]);
    } else {
      events_children.push_back(prime_to_int_[it_children->first]);
    }
  }

  // Type dependent logic.
  if (gate == "or") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTree::SetOr_(events_children, sets);
    } else {
      FaultTree::SetAnd_(events_children, sets, -1);
    }
  } else if (gate == "and") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTree::SetAnd_(events_children, sets);
    } else {
      FaultTree::SetOr_(events_children, sets, -1);
    }
  } else if (gate == "not") {
    int mult = 1;
    if (inter_index < 0) mult = -1;
    // Only one child is expected.
    assert(events_children.size() == 1);
    FaultTree::SetAnd_(events_children, sets, -1 * mult);
  } else if (gate == "nor") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTree::SetAnd_(events_children, sets, -1);
    } else {
      FaultTree::SetOr_(events_children, sets);
    }
  } else if (gate == "nand") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTree::SetOr_(events_children, sets, -1);
    } else {
      FaultTree::SetAnd_(events_children, sets);
    }
  } else if (gate == "xor") {
    assert(events_children.size() == 2);
    SupersetPtr tmp_set_one(new scram::Superset());
    SupersetPtr tmp_set_two(new scram::Superset());
    if (inter_index > 0) {
      int j = 1;
      for (it_child = events_children.begin();
           it_child != events_children.end(); ++it_child) {
        if (*it_child > top_event_index_) {
          tmp_set_one->AddInter(j * (*it_child));
          tmp_set_two->AddInter(-1 * j * (*it_child));
        } else {
          tmp_set_one->AddPrimary(j * (*it_child));
          tmp_set_two->AddPrimary(-1 * j * (*it_child));
        }
        j = -1;
      }
    } else {
      for (it_child = events_children.begin();
           it_child != events_children.end(); ++it_child) {
        if (*it_child > top_event_index_) {
          tmp_set_one->AddInter(*it_child);
          tmp_set_two->AddInter(-1 * (*it_child));
        } else {
          tmp_set_one->AddPrimary(*it_child);
          tmp_set_two->AddPrimary(-1 * (*it_child));
        }
      }
    }
    sets.push_back(tmp_set_one);
    sets.push_back(tmp_set_two);
  } else if (gate == "null") {
    int mult = 1;
    if (inter_index < 0) mult = -1;
    // Only one child is expected.
    assert(events_children.size() == 1);
    FaultTree::SetAnd_(events_children, sets, mult);
  } else if (gate == "inhibit") {
    assert(events_children.size() == 2);
    if (inter_index > 0) {
      FaultTree::SetAnd_(events_children, sets);
    } else {
      FaultTree::SetOr_(events_children, sets, -1);
    }
  } else if (gate == "vote") {
    int vote_number = int_to_inter_[std::abs(inter_index)]->vote_number();
    assert(vote_number > 1);
    assert(events_children.size() >= vote_number);
    std::set< std::set<int> > all_sets;
    int size = events_children.size();

    for (int j = 0; j < size; ++j) {
      std::set<int> set;
      set.insert(events_children[j]);
      all_sets.insert(set);
    }

    int mult = 1;
    if (inter_index < 0) {
      mult = -1;
      vote_number = size - vote_number + 1;  // The main trick for negation.
    }

    for (int i = 1; i < vote_number; ++i) {
      std::set< std::set<int> > tmp_sets;
      std::set< std::set<int> >::iterator it_sets;
      for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
        for (int j = 0; j < size; ++j) {
          std::set<int> set = *it_sets;
          set.insert(events_children[j]);
          if (set.size() > i) {
            tmp_sets.insert(set);
          }
        }
      }
      all_sets = tmp_sets;
    }

    std::set< std::set<int> >::iterator it_sets;
    for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
      SupersetPtr tmp_set_c(new scram::Superset());
      std::set<int>::iterator it;
      for (it = it_sets->begin(); it != it_sets->end(); ++it) {
        if (*it > top_event_index_) {
          tmp_set_c->AddInter(*it * mult);
        } else {
          tmp_set_c->AddPrimary(*it * mult);
        }
      }
      sets.push_back(tmp_set_c);
    }

  } else {
    boost::to_upper(gate);
    std::string msg = "No algorithm defined for " + gate;
    throw scram::ValueError(msg);
  }
}

void FaultTree::SetOr_(std::vector<int>& events_children,
                       std::vector<SupersetPtr>& sets, int mult) {
  std::vector<int>::iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    SupersetPtr tmp_set_c(new scram::Superset());
    if (*it_child > top_event_index_) {
      tmp_set_c->AddInter(*it_child * mult);
    } else {
      tmp_set_c->AddPrimary(*it_child * mult);
    }
    sets.push_back(tmp_set_c);
  }
}

void FaultTree::SetAnd_(std::vector<int>& events_children,
                        std::vector<SupersetPtr>& sets, int mult) {
  SupersetPtr tmp_set_c(new scram::Superset());
  std::vector<int>::iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    if (*it_child > top_event_index_) {
      tmp_set_c->AddInter(*it_child * mult);
    } else {
      tmp_set_c->AddPrimary(*it_child * mult);
    }
  }
  sets.push_back(tmp_set_c);
}

void FaultTree::FindMCS_(const std::set< std::set<int> >& cut_sets,
                         const std::set< std::set<int> >& mcs_lower_order,
                         int min_order) {
  if (cut_sets.empty()) return;

  // Iterator for cut_sets.
  std::set< std::set<int> >::iterator it_uniq;

  // Iterator for minimal cut sets.
  std::set< std::set<int> >::iterator it_min;

  std::set< std::set<int> > temp_sets;  // For mcs of a level above.
  std::set< std::set<int> > temp_min_sets;  // For mcs of this level.

  for (it_uniq = cut_sets.begin();
       it_uniq != cut_sets.end(); ++it_uniq) {
    bool include = true;  // Determine to keep or not.

    for (it_min = mcs_lower_order.begin(); it_min != mcs_lower_order.end();
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
      if (it_uniq->size() == min_order) {
        temp_min_sets.insert(*it_uniq);
        // Update maximum order of the sets.
        if (min_order > max_order_) max_order_ = min_order;
      } else {
        temp_sets.insert(*it_uniq);
      }
    }
    // Ignore the cut set because include = false.
  }
  imcs_.insert(temp_min_sets.begin(), temp_min_sets.end());
  min_order++;
  FaultTree::FindMCS_(temp_sets, temp_min_sets, min_order);
}

// -------------------- Algorithm for Cut Sets and Probabilities -----------
void FaultTree::AssignIndices_() {
  // Assign an index to each primary event, and populate relevant
  // databases.
  int j = 1;
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator itp;
  // Dummy primary event at index 0.
  int_to_prime_.push_back(PrimaryEventPtr(new PrimaryEvent("dummy")));
  iprobs_.push_back(0);
  for (itp = primary_events_.begin(); itp != primary_events_.end(); ++itp) {
    int_to_prime_.push_back(itp->second);
    prime_to_int_.insert(std::make_pair(itp->second->id(), j));
    if (prob_requested_) iprobs_.push_back(itp->second->p());
    ++j;
  }

  // Assign an index to each top and intermediate event and populate
  // relevant databases.
  top_event_index_ = j;
  int_to_inter_.insert(std::make_pair(j, top_event_));
  inter_to_int_.insert(std::make_pair(top_event_id_, j));
  ++j;
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
      if (*it_set < 0) {  // NOT logic.
        pr_set.insert("not " + int_to_prime_[std::abs(*it_set)]->id());
      } else {
        pr_set.insert(int_to_prime_[*it_set]->id());
      }
    }
    imcs_to_smcs_.insert(std::make_pair(*it_min, pr_set));
    min_cut_sets_.insert(pr_set);
  }
}

double FaultTree::ProbOr_(std::set< std::set<int> >& min_cut_sets, int nsums) {
  // Recursive implementation.
  if (min_cut_sets.empty()) return 0;

  if (nsums == 0) return 0;

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
  if (min_cut_set.empty()) return 0;

  double p_sub_set = 1;  // 1 is for multiplication.
  std::set<int>::iterator it_set;
  for (it_set = min_cut_set.begin(); it_set != min_cut_set.end(); ++it_set) {
    if (*it_set > 0) {
      p_sub_set *= iprobs_[*it_set];
    } else {
      p_sub_set *= 1 - iprobs_[std::abs(*it_set)];
    }
  }
  return p_sub_set;
}

void FaultTree::CombineElAndSet_(const std::set<int>& el,
                                 const std::set< std::set<int> >& set,
                                 std::set< std::set<int> >& combo_set) {
  std::set<int> member_set;
  std::set<int>::iterator it;
  std::set< std::set<int> >::iterator it_set;
  for (it_set = set.begin(); it_set != set.end(); ++it_set) {
    member_set = *it_set;
    bool include = true;
    for (it = el.begin(); it != el.end(); ++it) {
      if (it_set->count(-1 * (*it))) {
        include = false;
        break;
      }
      member_set.insert(*it);
    }
    if (include) combo_set.insert(member_set);
  }
}

// ----------------------------------------------------------------------
// ----- Algorithm for Total Equation for Monte Carlo Simulation --------
// Generation of the representation of the original equation.
void FaultTree::MProbOr_(std::set< std::set<int> >& min_cut_sets,
                         int sign, int nsums) {
  // Recursive implementation.
  if (min_cut_sets.empty()) return;

  if (nsums == 0) return;

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

  std::set< std::set<int> > combo_sets;
  FaultTree::CombineElAndSet_(element_one, min_cut_sets, combo_sets);
  FaultTree::MProbOr_(min_cut_sets, sign, nsums);
  FaultTree::MProbOr_(combo_sets, sign + 1, nsums - 1);
  return;
}

void FaultTree::MSample_() {}
// ----------------------------------------------------------------------

}  // namespace scram
