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

namespace scram {

FaultTreeDescriptor::FaultTreeDescriptor(const GatePtr& root)
    : top_event_(root) {
  FaultTreeDescriptor::GatherEvents(top_event_);
  FaultTreeDescriptor::ClearMarks();
}

void FaultTreeDescriptor::GatherEvents(const GatePtr& gate) noexcept {
  if (gate->mark() == "visited") return;
  gate->mark("visited");
  FaultTreeDescriptor::GatherEvents(gate->formula());
}

void FaultTreeDescriptor::GatherEvents(const FormulaPtr& formula) noexcept {
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
    FaultTreeDescriptor::GatherEvents(gate);
  }
  for (const FormulaPtr& arg : formula->formula_args()) {
    FaultTreeDescriptor::GatherEvents(arg);
  }
}

void FaultTreeDescriptor::ClearMarks() noexcept {
  top_event_->mark("");
  for (const std::pair<std::string, GatePtr>& member : inter_events_) {
    member.second->mark("");
  }
}

FaultTreeAnalysis::FaultTreeAnalysis(const GatePtr& root,
                                     const Settings& settings)
    : Analysis::Analysis(settings),
      FaultTreeDescriptor::FaultTreeDescriptor(root),
      sum_mcs_probability_(0),
      max_order_(0) {}

void FaultTreeAnalysis::SetsToString(const std::vector<std::vector<int>>& imcs,
                                     const BooleanGraph* ft) noexcept {
  using BasicEventPtr = std::shared_ptr<BasicEvent>;
  // Special cases of sets.
  if (imcs.empty()) {
    // Special case of null of a top event. No minimal cut sets found.
    warnings_ += " The top event is NULL. Success is guaranteed.";
  } else if (imcs.size() == 1 && imcs.back().empty()) {
    // Special case of unity of a top event.
    warnings_ += " The top event is UNITY. Failure is guaranteed.";
    max_order_ = 1;
  }
  assert(!sum_mcs_probability_);
  for (const auto& min_cut_set : imcs) {
    if (min_cut_set.size() > max_order_) max_order_ = min_cut_set.size();
    CutSet pr_set;
    double prob = 1;  // 1 is for multiplication and Unity set.
    for (int index : min_cut_set) {
      BasicEventPtr basic_event = ft->GetBasicEvent(std::abs(index));
      if (index < 0) {  // NOT logic.
        if (kSettings_.probability_analysis()) prob *= 1 - basic_event->p();
        pr_set.insert("not " + basic_event->id());
      } else {
        if (kSettings_.probability_analysis()) prob *= basic_event->p();
        pr_set.insert(basic_event->id());
      }
      mcs_basic_events_.emplace(basic_event->id(), basic_event);
    }
    min_cut_sets_.insert(pr_set);
    if (kSettings_.probability_analysis()) {
      mcs_probability_.emplace(pr_set, prob);
      sum_mcs_probability_ += prob;
    }
  }
}

}  // namespace scram
