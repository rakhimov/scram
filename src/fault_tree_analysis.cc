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

/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.

#include "fault_tree_analysis.h"

#include <set>
#include <utility>

namespace scram {

void Print(const std::vector<Product>& products) {
  if (products.empty()) {
    std::cerr << "No products!" << std::endl;
    return;
  }
  if (products.front().empty()) {
    assert(products.size() == 1 && "Unity case must have only one product.");
    std::cerr << "Single Unity product." << std::endl;
    return;
  }
  std::vector<std::set<std::string>> to_print;
  for (const auto& product : products) {
    std::set<std::string> ids;
    for (const auto& literal : product) {
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
  for (const auto& product : to_print) distribution[product.size() - 1]++;
  std::cerr << " " << to_print.size() << " : {";
  for (int i : distribution) std::cerr << " " << i;
  std::cerr << " }" << std::endl << std::endl;

  for (const auto& product : to_print) {
    for (const auto& id : product) std::cerr << " " << id;
    std::cerr << std::endl;
  }
}

double CalculateProbability(const Product& product) {
  double p = 1;
  for (const Literal& literal : product) {
    p *= literal.complement ? 1 - literal.event->p() : literal.event->p();
  }
  return p;
}

int GetOrder(const Product& product) {
  return product.empty() ? 1 : product.size();
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
    : Analysis(settings),
      FaultTreeDescriptor(root) {}

void FaultTreeAnalysis::Convert(const std::vector<std::vector<int>>& results,
                                const BooleanGraph* graph) noexcept {
  // Special cases of sets.
  if (results.empty()) {
    Analysis::AddWarning("The top event is NULL. Success is guaranteed.");
  } else if (results.size() == 1 && results.back().empty()) {
    Analysis::AddWarning("The top event is UNITY. Failure is guaranteed.");
  }
  std::unordered_set<int> unique_events;
  for (const auto& result_set : results) {
    assert(result_set.size() <= Analysis::settings().limit_order() &&
           "Miscalculated product sets with larger-than-required order.");
    Product product;
    product.reserve(result_set.size());
    for (int index : result_set) {
      int abs_index = std::abs(index);
      const BasicEventPtr& basic_event = graph->GetBasicEvent(abs_index);
      product.push_back({index < 0, basic_event});
      if (unique_events.count(abs_index)) continue;
      unique_events.insert(abs_index);
      product_events_.push_back(basic_event);
    }
    products_.emplace_back(std::move(product));
  }
#ifndef NDEBUG
  if (Analysis::settings().print) Print(products_);
#endif
}

}  // namespace scram
