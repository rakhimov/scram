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

#include <algorithm>
#include <utility>

#include <boost/container/flat_set.hpp>
#include <boost/range/algorithm.hpp>

namespace scram {
namespace core {

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
  using ProductSet = boost::container::flat_set<std::string>;
  std::vector<ProductSet> to_print;
  for (const auto& product : products) {
    ProductSet ids;
    for (const auto& literal : product) {
      ids.insert((literal.complement ? "~" : "") + literal.event.name());
    }
    to_print.push_back(std::move(ids));
  }
  boost::sort(to_print, [](const ProductSet& lhs, const ProductSet& rhs) {
    if (lhs.size() == rhs.size()) return lhs < rhs;
    return lhs.size() < rhs.size();
  });
  assert(!to_print.front().empty() && "Failure of the analysis with Unity!");
  std::vector<int> distribution(to_print.back().size());
  for (const auto& product : to_print) distribution[product.size() - 1]++;
  std::cerr << " " << to_print.size() << " : {";
  for (int i : distribution) std::cerr << " " << i;
  std::cerr << " }\n\n";

  for (const auto& product : to_print) {
    for (const auto& id : product) std::cerr << " " << id;
    std::cerr << "\n";
  }
  std::cerr << std::flush;
}

double CalculateProbability(const Product& product) {
  double p = 1;
  for (const Literal& literal : product) {
    p *= literal.complement ? 1 - literal.event.p() : literal.event.p();
  }
  return p;
}

int GetOrder(const Product& product) {
  return product.empty() ? 1 : product.size();
}

FaultTreeDescriptor::FaultTreeDescriptor(const mef::Gate& root)
    : top_event_(root) {
  FaultTreeDescriptor::GatherEvents(top_event_.formula());
}

void FaultTreeDescriptor::GatherEvents(const mef::Formula& formula) noexcept {
  for (const mef::BasicEventPtr& basic_event : formula.basic_event_args()) {
    basic_events_.emplace(basic_event->id(), basic_event.get());
    if (basic_event->HasCcf())
      ccf_events_.emplace(basic_event->id(), basic_event.get());
  }
  for (const mef::HouseEventPtr& house_event : formula.house_event_args()) {
    house_events_.emplace(house_event->id(), house_event.get());
  }
  for (const mef::GatePtr& gate : formula.gate_args()) {
    bool unvisited = inter_events_.emplace(gate->id(), gate.get()).second;
    if (unvisited) FaultTreeDescriptor::GatherEvents(gate->formula());
  }
  for (const mef::FormulaPtr& arg : formula.formula_args()) {
    FaultTreeDescriptor::GatherEvents(*arg);
  }
}

FaultTreeAnalysis::FaultTreeAnalysis(const mef::Gate& root,
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
  assert(products_.empty());
  products_.reserve(results.size());

  struct GeneratorIterator {
    void operator++() { ++it; }
    /// Populates the Product with Literals.
    std::pair<bool, const mef::BasicEvent*> operator*() {
      const mef::BasicEvent* basic_event = graph.GetBasicEvent(std::abs(*it));
      product_events.insert(basic_event);
      return {*it < 0, basic_event};
    }
    std::vector<int>::const_iterator it;
    const BooleanGraph& graph;
    decltype(product_events_)& product_events;
  };

  for (const auto& result_set : results) {
    assert(result_set.size() <= Analysis::settings().limit_order() &&
           "Miscalculated product sets with larger-than-required order.");
    products_.emplace_back(
        result_set.size(),
        GeneratorIterator{result_set.begin(), *graph, product_events_});
  }
#ifndef NDEBUG
  if (Analysis::settings().print) Print(products_);
#endif
}

}  // namespace core
}  // namespace scram
