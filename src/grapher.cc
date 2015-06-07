/// @file grapher.cc
/// Implements Grapher.
#include "grapher.h"

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/pointer_cast.hpp>

#include "error.h"

namespace fs = boost::filesystem;

namespace scram {

std::map<std::string, std::string> Grapher::gate_colors_ =
    boost::assign::map_list_of ("or", "blue") ("and", "green") ("not", "red")
                               ("xor", "brown") ("inhibit", "yellow")
                               ("atleast", "cyan") ("null", "gray")
                               ("nor", "magenta") ("nand", "orange");

std::map<std::string, std::string> Grapher::event_colors_ =
    boost::assign::map_list_of ("basic", "black") ("undeveloped", "blue")
                               ("house", "green") ("conditional", "red");

void Grapher::GraphFaultTree(const FaultTreePtr& fault_tree,
                             bool prob_requested,
                             std::ostream& out) {
  // The structure of the output:
  // List gates with their children following the tree structure.
  // List reused intermediate events as transfer gates.
  // List gates and primary events' descriptions.

  out << "digraph " << fault_tree->name() << " {\n";

  // Write top event.
  // Keep track of number of repetitions of the primary events.
  std::map<std::string, int> pr_repeat;
  // Keep track of number of repetitions of the intermediate events.
  std::map<std::string, int> in_repeat;
  // Populate intermediate and primary events of the top.
  Grapher::GraphNode(fault_tree->top_event(), fault_tree->primary_events(),
                     &pr_repeat, &in_repeat, out);
  // Do the same for all intermediate events.
  boost::unordered_map<std::string, GatePtr>::const_iterator it_inter;
  for (it_inter = fault_tree->inter_events().begin();
       it_inter != fault_tree->inter_events().end();
       ++it_inter) {
    Grapher::GraphNode(it_inter->second, fault_tree->primary_events(),
                       &pr_repeat, &in_repeat, out);
  }

  // Format events.
  Grapher::FormatTopEvent(fault_tree->top_event(), out);
  Grapher::FormatIntermediateEvents(fault_tree->inter_events(),
                                    in_repeat, out);
  Grapher::FormatPrimaryEvents(fault_tree->primary_events(), pr_repeat,
                               prob_requested, out);

  out << "}";
  out.flush();
}

void Grapher::GraphNode(
    const GatePtr& t,
    const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events,
    std::map<std::string, int>* pr_repeat,
    std::map<std::string, int>* in_repeat,
    std::ostream& out) {
  // Populate intermediate and primary events of the input inter event.
  std::map<std::string, EventPtr> events_children = t->children();
  std::map<std::string, EventPtr>::iterator it_child;
  for (it_child = events_children.begin(); it_child != events_children.end();
       ++it_child) {
    // Deal with repeated primary events.
    if (primary_events.count(it_child->first)) {
      if (pr_repeat->count(it_child->first)) {
        int rep = pr_repeat->find(it_child->first)->second;
        rep++;
        pr_repeat->erase(it_child->first);
        pr_repeat->insert(std::make_pair(it_child->first, rep));
      } else {
        pr_repeat->insert(std::make_pair(it_child->first, 0));
      }
      out << "\"" << t->name() << "_R0\" -> "
          << "\"" << it_child->second->name() <<"_R"
          << pr_repeat->find(it_child->first)->second << "\";\n";
    } else {  // This must be an intermediate event.
      if (in_repeat->count(it_child->first)) {
        int rep = in_repeat->find(it_child->first)->second;
        rep++;
        in_repeat->erase(it_child->first);
        in_repeat->insert(std::make_pair(it_child->first, rep));
      } else {
        in_repeat->insert(std::make_pair(it_child->first, 0));
      }
      out << "\"" << t->name() << "_R0\" -> "
          << "\"" << it_child->second->name() <<"_R"
          << in_repeat->find(it_child->first)->second << "\";\n";
    }
  }
}

void Grapher::FormatTopEvent(const GatePtr& top_event, std::ostream& out) {
  std::string gate = top_event->type();

  // Special case for inhibit gate.
  if (gate == "and" && top_event->HasAttribute("flavor"))
    gate = top_event->GetAttribute("flavor").value;

  std::string gate_color = "black";
  if (gate_colors_.count(gate)) {
    gate_color = gate_colors_.find(gate)->second;
  }

  boost::to_upper(gate);
  out << "\"" <<  top_event->name()
      << "_R0\" [shape=ellipse, "
      << "fontsize=12, fontcolor=black, fontname=\"times-bold\", "
      << "color=" << gate_color << ", "
      << "label=\"" << top_event->name() << "\\n"
      << "{ " << gate;
  if (gate == "ATLEAST") {
    out << " " << top_event->vote_number() << "/"
        << top_event->children().size();
  }
  out << " }\"]\n";
}

void Grapher::FormatIntermediateEvents(
    const boost::unordered_map<std::string, GatePtr>& inter_events,
    const std::map<std::string, int>& in_repeat,
    std::ostream& out) {
  std::map<std::string, int>::const_iterator it;
  for (it = in_repeat.begin(); it != in_repeat.end(); ++it) {
    std::string gate = inter_events.find(it->first)->second->type();

    if (inter_events.find(it->first)->second->HasAttribute("flavor") &&
        gate == "and")
      gate =
          inter_events.find(it->first)->second->GetAttribute("flavor").value;

    std::string gate_color = "black";
    if (gate_colors_.count(gate)) {
      gate_color = gate_colors_.find(gate)->second;
    }
    boost::to_upper(gate);  // This is for graphing.
    std::string type = inter_events.find(it->first)->second->type();
    std::string orig_name = inter_events.find(it->first)->second->name();
    for (int i = 0; i <= it->second; ++i) {
      if (i == 0) {
        out << "\"" <<  orig_name << "_R" << i
            << "\" [shape=box, ";
      } else {
        // Repetition is a transfer symbol.
        out << "\"" <<  orig_name << "_R" << i
            << "\" [shape=triangle, ";
      }
      out << "fontsize=10, fontcolor=black, "
          << "color=" << gate_color << ", "
          << "label=\"" << orig_name << "\\n"
          << "{ " << gate;
      if (gate == "ATLEAST") {
        out << " " << inter_events.find(it->first)->second->vote_number()
            << "/" << inter_events.find(it->first)->second->children().size();
      }
      out << " }\"]\n";
    }
  }
}

void Grapher::FormatPrimaryEvents(
    const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events,
    const std::map<std::string, int>& pr_repeat,
    bool prob_requested,
    std::ostream& out) {
  std::map<std::string, int>::const_iterator it;
  for (it = pr_repeat.begin(); it != pr_repeat.end(); ++it) {
    for (int i = 0; i < it->second + 1; ++i) {
      PrimaryEventPtr primary_event = primary_events.find(it->first)->second;

      std::string type = primary_event->type();
      // Detect undeveloped or conditional event.
      if (type == "basic" && primary_event->HasAttribute("flavor")) {
        type = primary_event->GetAttribute("flavor").value;
      }

      out << "\"" << primary_event->name() << "_R" << i
          << "\" [shape=circle, "
          << "height=1, fontsize=10, fixedsize=true, "
          << "fontcolor=" << event_colors_.find(type)->second
          << ", " << "label=\"" << primary_event->name() << "\\n["
          << type << "]";

      if (prob_requested) {
        out << "\\n";
        if (type == "house") {
          std::string state =
              boost::dynamic_pointer_cast<HouseEvent>(primary_event)->state() ?
              "True" : "False";
          out << state;
        } else {
          // Note that this might a flavored type of a basic event.
          double p =
              boost::dynamic_pointer_cast<BasicEvent>(primary_event)->p();
          out << p;
        }
      }
      out << "\"]\n";
    }
  }
}

}  // namespace scram
