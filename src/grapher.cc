/// @file grapher.cc
/// Implements Grapher.
#include "grapher.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>

namespace fs = boost::filesystem;

namespace scram {

Grapher::Grapher() : prob_requested_(false) {}

void Grapher::GraphFaultTree(
    const FaultTreePtr& fault_tree,
    const std::map<std::string, std::string>& orig_ids,
    bool prob_requested,
    std::string output) {
  // List inter events and their children.
  // List inter events and primary events' descriptions.
  // Getting events from the fault tree object.

  /// @todo Option to graph re-used gates as a separate transfer trees.
  top_event_ = fault_tree->top_event();
  inter_events_ = fault_tree->inter_events();
  primary_events_ = fault_tree->primary_events();
  orig_ids_ = orig_ids;
  prob_requested_ = prob_requested;

  std::string graph_name = "fault_tree.dot";
  if (output != "" ) graph_name = output;
  graph_name.erase(graph_name.find_last_of("."), std::string::npos);

  std::string output_path = graph_name + ".dot";

  graph_name = graph_name.substr(graph_name.find_last_of("/") +
                                 1, std::string::npos);
  std::ofstream out(output_path.c_str());
  if (!out.good()) {
    throw scram::IOError(output_path +  " : Cannot write the graphing file.");
  }

  boost::to_upper(graph_name);
  out << "digraph " << graph_name << " {\n";

  // Write top event.
  // Keep track of number of repetitions of the primary events.
  std::map<std::string, int> pr_repeat;
  // Keep track of number of repetitions of the intermediate events.
  std::map<std::string, int> in_repeat;
  // Populate intermediate and primary events of the top.
  Grapher::GraphNode(top_event_, pr_repeat, in_repeat, out);
  out.flush();
  // Do the same for all intermediate events.
  boost::unordered_map<std::string, GatePtr>::iterator it_inter;
  for (it_inter = inter_events_.begin(); it_inter != inter_events_.end();
       ++it_inter) {
    Grapher::GraphNode(it_inter->second, pr_repeat, in_repeat, out);
    out.flush();
  }

  // Format events.
  std::map<std::string, std::string> gate_colors;
  gate_colors.insert(std::make_pair("or", "blue"));
  gate_colors.insert(std::make_pair("and", "green"));
  gate_colors.insert(std::make_pair("not", "red"));
  gate_colors.insert(std::make_pair("xor", "brown"));
  gate_colors.insert(std::make_pair("inhibit", "yellow"));
  gate_colors.insert(std::make_pair("vote", "cyan"));
  gate_colors.insert(std::make_pair("atleast", "cyan"));
  gate_colors.insert(std::make_pair("null", "gray"));
  gate_colors.insert(std::make_pair("nor", "magenta"));
  gate_colors.insert(std::make_pair("nand", "orange"));
  std::string gate = top_event_->type();
  std::string gate_color = "black";
  if (gate_colors.count(gate)) {
    gate_color = gate_colors.find(gate)->second;
  }
  boost::to_upper(gate);
  out << "\"" <<  orig_ids_.find(top_event_->id())->second << "_R0\" [shape=ellipse, "
      << "fontsize=12, fontcolor=black, fontname=\"times-bold\", "
      << "color=" << gate_color << ", "
      << "label=\"" << orig_ids_.find(top_event_->id())->second << "\\n"
      << "{ " << gate;
  if (gate == "VOTE" || gate == "ATLEAST") {
    out << " " << top_event_->vote_number() << "/"
        << top_event_->children().size();
  }
  out << " }\"]\n";

  std::map<std::string, int>::iterator it;
  for (it = in_repeat.begin(); it != in_repeat.end(); ++it) {
    gate = inter_events_.find(it->first)->second->type();
    std::string gate_color = "black";
    if (gate_colors.count(gate)) {
      gate_color = gate_colors.find(gate)->second;
    }
    boost::to_upper(gate);  // This is for graphing.
    std::string type = inter_events_.find(it->first)->second->type();
    for (int i = 0; i <= it->second; ++i) {
      if (i == 0) {
        out << "\"" <<  orig_ids_.find(it->first)->second << "_R" << i << "\" [shape=box, ";
      } else {
        // Repetition is a transfer symbol.
        out << "\"" <<  orig_ids_.find(it->first)->second << "_R" << i
            << "\" [shape=triangle, ";
      }
      out << "fontsize=10, fontcolor=black, "
          << "color=" << gate_color << ", "
          << "label=\"" << orig_ids_.find(it->first)->second << "\\n"
          << "{ " << gate;
      if (gate == "VOTE" || gate == "ATLEAST") {
        out << " " << inter_events_.find(it->first)->second->vote_number() << "/"
            << inter_events_.find(it->first)->second->children().size();
      }
      out << " }\"]\n";
    }
  }
  out.flush();

  std::map<std::string, std::string> event_colors;
  event_colors.insert(std::make_pair("basic", "black"));
  event_colors.insert(std::make_pair("undeveloped", "blue"));
  event_colors.insert(std::make_pair("house", "green"));
  event_colors.insert(std::make_pair("conditional", "red"));
  for (it = pr_repeat.begin(); it != pr_repeat.end(); ++it) {
    for (int i = 0; i < it->second + 1; ++i) {
      out << "\"" << orig_ids_.find(it->first)->second << "_R" << i << "\" [shape=circle, "
          << "height=1, fontsize=10, fixedsize=true, "
          << "fontcolor=" << event_colors.find(primary_events_.find(it->first)->second->type())->second
          << ", " << "label=\"" << orig_ids_.find(it->first)->second << "\\n["
          << primary_events_.find(it->first)->second->type() << "]";
      if (prob_requested_) { out << "\\n" << primary_events_.find(it->first)->second->p(); }
      out << "\"]\n";
    }
  }

  out << "}";
  out.flush();
}

void Grapher::GraphNode(GatePtr t, std::map<std::string, int>& pr_repeat,
                        std::map<std::string, int>& in_repeat,
                        std::ofstream& out) {
  // Populate intermediate and primary events of the input inter event.
  std::map<std::string, EventPtr> events_children = t->children();
  std::map<std::string, EventPtr>::iterator it_child;
  for (it_child = events_children.begin(); it_child != events_children.end();
       ++it_child) {
    // Deal with repeated primary events.
    if (primary_events_.count(it_child->first)) {
      if (pr_repeat.count(it_child->first)) {
        int rep = pr_repeat.find(it_child->first)->second;
        rep++;
        pr_repeat.erase(it_child->first);
        pr_repeat.insert(std::make_pair(it_child->first, rep));
      } else {
        pr_repeat.insert(std::make_pair(it_child->first, 0));
      }
      out << "\"" <<  orig_ids_.find(t->id())->second << "_R0\" -> "
          << "\"" <<orig_ids_.find(it_child->first)->second <<"_R"
          << pr_repeat.find(it_child->first)->second << "\";\n";
    } else {  // This must be an intermediate event.
      if (in_repeat.count(it_child->first)) {
        int rep = in_repeat.find(it_child->first)->second;
        rep++;
        in_repeat.erase(it_child->first);
        in_repeat.insert(std::make_pair(it_child->first, rep));
      } else {
        in_repeat.insert(std::make_pair(it_child->first, 0));
      }
      out << "\"" << orig_ids_.find(t->id())->second << "_R0\" -> "
          << "\"" <<orig_ids_.find(it_child->first)->second <<"_R"
          << in_repeat.find(it_child->first)->second << "\";\n";
    }
  }
}

}  // namespace scram
