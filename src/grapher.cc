/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file grapher.cc
/// Implementation of graphing of analysis constructs.

#include "grapher.h"

#include <boost/algorithm/string.hpp>

#include "fault_tree_analysis.h"

namespace scram {

const std::map<std::string, std::string> Grapher::kGateColors_ = {
    {"or", "blue"},
    {"and", "green"},
    {"not", "red"},
    {"xor", "brown"},
    {"inhibit", "yellow"},
    {"vote", "cyan"},
    {"null", "gray"},
    {"nor", "magenta"},
    {"nand", "orange"}};

const std::map<std::string, std::string> Grapher::kEventColors_ = {
    {"basic", "black"},
    {"undeveloped", "blue"},
    {"house", "green"},
    {"conditional", "red"}};

void Grapher::GraphFaultTree(const mef::GatePtr& top_event, bool prob_requested,
                             std::ostream& out) {
  // The structure of the output:
  // List gates with their children.
  // List common intermediate events as transfer symbols.
  // List gates and primary events' descriptions.

  out << "digraph " << top_event->name() << " {\n";

  auto* fta = new core::FaultTreeDescriptor(top_event);

  // Keep track of nested formulas for future special formatting.
  std::vector<std::pair<std::string, const mef::Formula*> > formulas;

  // Keep track of number of repetitions of nodes.
  // These repetitions are needed so that
  // the graph links to separate nodes with the same display name.
  std::unordered_map<mef::EventPtr, int> node_repeat;

  // Populate intermediate and primary events of the top.
  Grapher::GraphFormula(fta->top_event()->id() + "_R0",
                        fta->top_event()->formula(),
                        &formulas, &node_repeat, out);
  // Do the same for all intermediate events.
  for (auto it_inter = fta->inter_events().begin();
       it_inter != fta->inter_events().end(); ++it_inter) {
    std::string name = it_inter->second->id() + "_R0";
    const mef::FormulaPtr& formula = it_inter->second->formula();
    Grapher::GraphFormula(name, formula, &formulas, &node_repeat, out);
  }

  // Format events.
  Grapher::FormatTopEvent(fta->top_event(), out);
  Grapher::FormatIntermediateEvents(fta->inter_events(), node_repeat, out);
  Grapher::FormatBasicEvents(fta->basic_events(), node_repeat, prob_requested,
                             out);
  Grapher::FormatHouseEvents(fta->house_events(), node_repeat, prob_requested,
                             out);
  Grapher::FormatFormulas(formulas, out);

  out << "}\n";
  delete fta;
}

void Grapher::GraphFormula(
    const std::string& formula_name,
    const mef::FormulaPtr& formula,
    std::vector<std::pair<std::string, const mef::Formula*> >* formulas,
    std::unordered_map<mef::EventPtr, int>* node_repeat,
    std::ostream& out) {
  // Populate intermediate and primary events of the input gate.
  const std::map<std::string, mef::EventPtr>* events = &formula->event_args();
  std::map<std::string, mef::EventPtr>::const_iterator it_child;
  for (it_child = events->begin(); it_child != events->end(); ++it_child) {
    if (node_repeat->count(it_child->second)) {
      node_repeat->find(it_child->second)->second++;
    } else {
      node_repeat->insert(std::make_pair(it_child->second, 0));
    }
    out << "\"" << formula_name << "\" -> "
        << "\"" << it_child->second->id() << "_R"
        << node_repeat->find(it_child->second)->second << "\";\n";
  }
  // Deal with formulas.
  int i = 1;  // Count formulas for unique naming.
  for (const mef::FormulaPtr& arg : formula->formula_args()) {
    std::stringstream unique_name;
    unique_name << formula_name << "._F" << i;  // Unique name.
    ++i;
    out << "\"" << formula_name << "\" -> "
        << "\"" << unique_name.str() << "\";\n";
    formulas->emplace_back(unique_name.str(), arg.get());
    Grapher::GraphFormula(unique_name.str(), arg, formulas, node_repeat, out);
  }
}

void Grapher::FormatTopEvent(const mef::GatePtr& top_event, std::ostream& out) {
  std::string gate = top_event->formula()->type();
  if (gate == "atleast") gate = "vote";
  // Special case for inhibit gate.
  if (gate == "and" && top_event->HasAttribute("flavor"))
    gate = top_event->GetAttribute("flavor").value;

  std::string gate_color = kGateColors_.at(gate);

  boost::to_upper(gate);
  out << "\"" <<  top_event->id()
      << "_R0\" [shape=ellipse, "
      << "fontsize=12, fontcolor=black, fontname=\"times-bold\", "
      << "color=" << gate_color << ", "
      << "label=\"" << top_event->name() << "\\n";
  if (top_event->role() == mef::RoleSpecifier::kPrivate)
    out << "-- private --\\n";
  out << "{ " << gate;
  if (gate == "VOTE") {
    out << " " << top_event->formula()->vote_number()
        << "/" << top_event->formula()->num_args();
  }
  out << " }\"]\n";
}

void Grapher::FormatIntermediateEvents(
    const std::unordered_map<std::string, mef::GatePtr>& inter_events,
    const std::unordered_map<mef::EventPtr, int>& node_repeat,
    std::ostream& out) {
  std::unordered_map<std::string, mef::GatePtr>::const_iterator it;
  for (it = inter_events.begin(); it != inter_events.end(); ++it) {
    std::string gate = it->second->formula()->type();
    if (gate == "atleast") gate = "vote";
    if (it->second->HasAttribute("flavor") && gate == "and")
      gate = it->second->GetAttribute("flavor").value;

    std::string gate_color = kGateColors_.at(gate);
    boost::to_upper(gate);  // This is for graphing.
    std::string id = it->second->id();
    std::string name = it->second->name();
    int repetition = node_repeat.find(it->second)->second;
    for (int i = 0; i <= repetition; ++i) {
        out << "\"" << id << "_R" << i << "\"";
      if (i == 0) {
        out << " [shape=box, ";
      } else {
        // Repetition is a transfer symbol.
        out << " [shape=triangle, ";
      }
      out << "fontsize=10, fontcolor=black, "
          << "color=" << gate_color << ", "
          << "label=\"" << name << "\\n";  // This is a new line in the label.
      if (it->second->role() == mef::RoleSpecifier::kPrivate)
        out << "-- private --\\n";
      out << "{ " << gate;
      if (gate == "VOTE") {
        out << " " << it->second->formula()->vote_number()
            << "/" << it->second->formula()->num_args();
      }
      out << " }\"]\n";
    }
  }
}

void Grapher::FormatBasicEvents(
    const std::unordered_map<std::string, mef::BasicEventPtr>& basic_events,
    const std::unordered_map<mef::EventPtr, int>& node_repeat,
    bool prob_requested,
    std::ostream& out) {
  std::unordered_map<std::string, mef::BasicEventPtr>::const_iterator it;
  for (it = basic_events.begin(); it != basic_events.end(); ++it) {
    std::string prob_msg;
    if (prob_requested) {
      std::stringstream snippet;
      snippet << it->second->p();
      prob_msg = "\\n";
      prob_msg += snippet.str();
    }
    int repetition = node_repeat.find(it->second)->second;
    std::string type = "basic";
    // Detect undeveloped or conditional event.
    if (it->second->HasAttribute("flavor")) {
      type = it->second->GetAttribute("flavor").value;
    }
    Grapher::FormatPrimaryEvent(it->second, repetition, type, prob_msg, out);
  }
}

void Grapher::FormatHouseEvents(
    const std::unordered_map<std::string, mef::HouseEventPtr>& house_events,
    const std::unordered_map<mef::EventPtr, int>& node_repeat,
    bool prob_requested,
    std::ostream& out) {
  std::unordered_map<std::string, mef::HouseEventPtr>::const_iterator it;
  for (it = house_events.begin(); it != house_events.end(); ++it) {
    std::string prob_msg;
    if (prob_requested) {
      prob_msg = "\\n";
      prob_msg += it->second->state() ? "True" : "False";
    }
    int repetition = node_repeat.find(it->second)->second;
    Grapher::FormatPrimaryEvent(it->second, repetition, "house", prob_msg, out);
  }
}

void Grapher::FormatPrimaryEvent(const mef::PrimaryEventPtr& primary_event,
                                 int repetition,
                                 const std::string& type,
                                 const std::string& prob_msg,
                                 std::ostream& out) {
  std::string color = kEventColors_.find(type)->second;
  for (int i = 0; i <= repetition; ++i) {
    out << "\"" << primary_event->id() << "_R" << i
        << "\" [shape=circle, "
        << "height=1, fontsize=10, fixedsize=true, "
        << "fontcolor=" << color
        << ", " << "label=\"" << primary_event->name() << "\\n";
    if (primary_event->role() == mef::RoleSpecifier::kPrivate)
      out << "-- private --\\n";
    out << "[" << type << "]" << prob_msg << "\"]\n";
  }
}

void Grapher::FormatFormulas(
    const std::vector<std::pair<std::string, const mef::Formula*> >& formulas,
    std::ostream& out) {
  for (const auto& pair : formulas) {
    std::string gate = pair.second->type();
    std::string gate_color = kGateColors_.at(gate);
    boost::to_upper(gate);  // This is for graphing.
    out << "\"" << pair.first << "\"";
    out << " [shape=box, ";
    out << "fontsize=10, fontcolor=black, "
        << "color=" << gate_color << ", "
        << "label=\"{ " << gate;
    if (gate == "VOTE") {
      out << " " << pair.second->vote_number()
          << "/" << pair.second->num_args();
    }
    out << " }\"]\n";
  }
}

}  // namespace scram
