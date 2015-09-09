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

#include <utility>

#include <boost/algorithm/string.hpp>

#include "boolean_graph.h"
#include "error.h"
#include "logger.h"
#include "preprocessor.h"

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(const GatePtr& root,
                                     const Settings& settings)
    : top_event_(root),
      kSettings_(settings),
      warnings_(""),
      max_order_(0),
      analysis_time_(0) {
  FaultTreeAnalysis::GatherEvents(top_event_);
  FaultTreeAnalysis::CleanMarks();
}

void FaultTreeAnalysis::Analyze() noexcept {
  CLOCK(analysis_time);

  CLOCK(ft_creation);
  BooleanGraph* indexed_tree = new BooleanGraph(top_event_,
                                                kSettings_.ccf_analysis());
  LOG(DEBUG2) << "Boolean graph is created in " << DUR(ft_creation);

  CLOCK(prep_time);  // Overall preprocessing time.
  LOG(DEBUG2) << "Preprocessing...";
  Preprocessor* preprocessor = new Preprocessor(indexed_tree);
  preprocessor->ProcessFaultTree();
  delete preprocessor;  // No exceptions are expected.
  LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);

  Mocus* mocus = new Mocus(indexed_tree, kSettings_.limit_order());
  mocus->FindMcs();

  const std::vector<Set>& imcs = mocus->GetGeneratedMcs();
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

void FaultTreeAnalysis::GatherEvents(const GatePtr& gate) noexcept {
  if (gate->mark() == "visited") return;
  gate->mark("visited");
  FaultTreeAnalysis::GatherEvents(gate->formula());
}

void FaultTreeAnalysis::GatherEvents(const FormulaPtr& formula) noexcept {
  for (const BasicEventPtr& basic_event : formula->basic_event_args()) {
    basic_events_.emplace(basic_event->id(), basic_event);
    if (basic_event->HasCcf())
      ccf_events_.emplace(basic_event->id(), basic_event);
  }
  for (const HouseEventPtr& house_event : formula->house_event_args()) {
    house_events_.emplace(house_event->id(), house_event);
  }
  for (const GatePtr& gate : formula->gate_args()) {
    inter_events_.emplace(gate->id(), gate);
    FaultTreeAnalysis::GatherEvents(gate);
  }
  for (const FormulaPtr& arg : formula->formula_args()) {
    FaultTreeAnalysis::GatherEvents(arg);
  }
}

void FaultTreeAnalysis::CleanMarks() noexcept {
  top_event_->mark("");
  for (const std::pair<std::string, GatePtr>& member : inter_events_) {
    member.second->mark("");
  }
}

void FaultTreeAnalysis::SetsToString(const std::vector<Set>& imcs,
                                     const BooleanGraph* ft) noexcept {
  for (const Set& min_cut_set : imcs) {
    if (min_cut_set.size() > max_order_) max_order_ = min_cut_set.size();
    std::set<std::string> pr_set;
    for (int index : min_cut_set) {
      BasicEventPtr basic_event = ft->GetBasicEvent(std::abs(index));
      if (index < 0) {  // NOT logic.
        pr_set.insert("not " + basic_event->id());
      } else {
        pr_set.insert(basic_event->id());
      }
      mcs_basic_events_.emplace(basic_event->id(), basic_event);
    }
    min_cut_sets_.insert(pr_set);
  }
}

}  // namespace scram
