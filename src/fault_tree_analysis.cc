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

void Print(const std::vector<CutSet>& cut_sets) {
  if (cut_sets.empty()) {
    std::cerr << "No cut sets!" << std::endl;
    return;
  }
  if (cut_sets.front().empty()) {
    assert(cut_sets.size() == 1 && "Unity case must have only one cut set.");
    std::cerr << "Single Unity cut set." << std::endl;
    return;
  }
  std::vector<std::set<std::string>> to_print;
  for (const auto& cut_set : cut_sets) {
    std::set<std::string> ids;
    for (const auto& literal : cut_set) {
      ids.insert((literal.complement ? "~" : "") + literal.event->name());
    }
    to_print.push_back(ids);
  }
  std::sort(
      to_print.begin(), to_print.end(),
      [](const std::set<std::string>& lhs, const std::set<std::string>& rhs) {
        if (lhs.size() == rhs.size()) return lhs < rhs;
        return lhs.size() < rhs.size();
      });
  assert(!to_print.front().empty() && "Failure of the analysis with Unity!");
  std::vector<int> distribution(to_print.back().size());
  for (const auto& cut_set : to_print) distribution[cut_set.size() - 1]++;
  std::cerr << " " << to_print.size() << " : {";
  for (int i : distribution) std::cerr << " " << i;
  std::cerr << " }" << std::endl << std::endl;

  for (const auto& cut_set : to_print) {
    for (const auto& id : cut_set) {
      std::cerr << " " << id;
    }
    std::cerr << std::endl;
  }
}

double CalculateProbability(const CutSet& cut_set) {
  double p = 1;
  for (const Literal& literal : cut_set) {
    p *= literal.complement ? 1 - literal.event->p() : literal.event->p();
  }
  return p;
}

int GetOrder(const CutSet& cut_set) {
  return cut_set.empty() ? 1 : cut_set.size();
}

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
  for (const std::pair<const std::string, GatePtr>& member : inter_events_) {
    member.second->mark("");
  }
}

FaultTreeAnalysis::FaultTreeAnalysis(const GatePtr& root,
                                     const Settings& settings)
    : Analysis::Analysis(settings),
      FaultTreeDescriptor::FaultTreeDescriptor(root) {}

void FaultTreeAnalysis::Convert(const std::vector<std::vector<int>>& i_cut_sets,
                                const BooleanGraph* graph) noexcept {
  // Special cases of sets.
  if (i_cut_sets.empty()) {
    warnings_ += " The top event is NULL. Success is guaranteed.";
  } else if (i_cut_sets.size() == 1 && i_cut_sets.back().empty()) {
    warnings_ += " The top event is UNITY. Failure is guaranteed.";
  }
  std::unordered_set<int> unique_events;
  for (const auto& min_cut_set : i_cut_sets) {
    assert(min_cut_set.size() <= kSettings_.limit_order() &&
           "Miscalculated cut sets with larger-than-required order.");
    CutSet result;
    for (int index : min_cut_set) {
      BasicEventPtr basic_event = graph->GetBasicEvent(std::abs(index));
      if (index < 0) {  // NOT logic.
        result.push_back({true, basic_event});
      } else {
        result.push_back({false, basic_event});
      }
      if (unique_events.count(std::abs(index))) continue;
      unique_events.insert(std::abs(index));
      cut_set_events_.push_back(basic_event);
    }
    cut_sets_.push_back(result);
  }
#ifndef NDEBUG
  if (kSettings_.print) Print(cut_sets_);
#endif
}

}  // namespace scram
