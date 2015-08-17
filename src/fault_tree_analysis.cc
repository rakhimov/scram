/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.

#include "fault_tree_analysis.h"

#include <boost/algorithm/string.hpp>

#include "boolean_graph.h"
#include "error.h"
#include "logger.h"
#include "mocus.h"
#include "preprocessor.h"

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(const GatePtr& root, int limit_order,
                                     bool ccf_analysis)
    : ccf_analysis_(ccf_analysis),
      warnings_(""),
      max_order_(0),
      analysis_time_(0) {
  // Check for the right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw InvalidArgument(msg);
  }
  limit_order_ = limit_order;
  top_event_ = root;
  FaultTreeAnalysis::GatherEvents(top_event_);
  FaultTreeAnalysis::CleanMarks();
}

void FaultTreeAnalysis::Analyze() {
  CLOCK(analysis_time);

  CLOCK(ft_creation);
  BooleanGraph* indexed_tree = new BooleanGraph(top_event_, ccf_analysis_);
  LOG(DEBUG2) << "Indexed fault tree is created in " << DUR(ft_creation);

  CLOCK(prep_time);  // Overall preprocessing time.
  LOG(DEBUG2) << "Preprocessing...";
  Preprocessor* preprocessor = new Preprocessor(indexed_tree);
  preprocessor->ProcessFaultTree();
  delete preprocessor;  // No exceptions are expected.
  LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);

  Mocus* mocus = new Mocus(indexed_tree, limit_order_);
  mocus->FindMcs();

  const std::vector< std::set<int> >& imcs = mocus->GetGeneratedMcs();
  // Special cases of sets.
  if (imcs.empty()) {
    // Special case of null of a top event. No minimal cut sets found.
    warnings_ += " The top event is NULL. Success is guaranteed.";
  } else if (imcs.size() == 1 && imcs.back().empty()) {
    // Special case of unity of a top event.
    warnings_ += " The top event is UNITY. Failure is guaranteed.";
  }

  analysis_time_ = DUR(analysis_time);  // Duration of MCS generation.
  FaultTreeAnalysis::SetsToString(imcs, indexed_tree);  // MCS with event ids.
  delete indexed_tree;  // No exceptions are expected.
  delete mocus;  // No exceptions are expected.
}

void FaultTreeAnalysis::GatherEvents(const GatePtr& gate) {
  if (gate->mark() == "visited") return;
  gate->mark("visited");
  FaultTreeAnalysis::GatherEvents(gate->formula());
}

void FaultTreeAnalysis::GatherEvents(const FormulaPtr& formula) {
  std::vector<BasicEventPtr>::const_iterator it_b;
  for (it_b = formula->basic_event_args().begin();
       it_b != formula->basic_event_args().end(); ++it_b) {
    BasicEventPtr basic_event = *it_b;
    basic_events_.insert(std::make_pair(basic_event->id(), basic_event));
    if (basic_event->HasCcf())
      ccf_events_.insert(std::make_pair(basic_event->id(), basic_event));
  }
  std::vector<HouseEventPtr>::const_iterator it_h;
  for (it_h = formula->house_event_args().begin();
       it_h != formula->house_event_args().end(); ++it_h) {
    HouseEventPtr house_event = *it_h;
    house_events_.insert(std::make_pair(house_event->id(), house_event));
  }
  std::vector<GatePtr>::const_iterator it_g;
  for (it_g = formula->gate_args().begin();
       it_g != formula->gate_args().end(); ++it_g) {
    GatePtr gate = *it_g;
    inter_events_.insert(std::make_pair(gate->id(), gate));
    FaultTreeAnalysis::GatherEvents(gate);
  }
  const std::set<FormulaPtr>* formulas = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas->begin(); it_f != formulas->end(); ++it_f) {
    FaultTreeAnalysis::GatherEvents(*it_f);
  }
}

void FaultTreeAnalysis::CleanMarks() {
  top_event_->mark("");
  std::unordered_map<std::string, GatePtr>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    it->second->mark("");
  }
}

void FaultTreeAnalysis::SetsToString(const std::vector< std::set<int> >& imcs,
                                     const BooleanGraph* ft) {
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs.begin(); it_min != imcs.end(); ++it_min) {
    if (it_min->size() > max_order_) max_order_ = it_min->size();
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      BasicEventPtr basic_event = ft->GetBasicEvent(std::abs(*it_set));
      if (*it_set < 0) {  // NOT logic.
        pr_set.insert("not " + basic_event->id());
      } else {
        pr_set.insert(basic_event->id());
      }
      mcs_basic_events_.insert(std::make_pair(basic_event->id(), basic_event));
    }
    min_cut_sets_.insert(pr_set);
  }
}

}  // namespace scram
