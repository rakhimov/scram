/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

#include <iostream>
#include <string>

#include <boost/container/flat_set.hpp>
#include <boost/range/algorithm.hpp>

#include "event.h"
#include "logger.h"

namespace scram {
namespace core {

void Print(const ProductContainer& products) {
  if (products.empty()) {
    std::cerr << "No products!" << std::endl;
    return;
  }
  if (products.begin()->empty()) {
    assert(products.size() == 1 && "Unity case must have only one product.");
    std::cerr << "Single Unity product." << std::endl;
    return;
  }
  std::cerr << " " << products.size() << " : {";
  for (int i : products.Distribution())
    std::cerr << " " << i;
  std::cerr << " }\n\n";

  using ProductSet = boost::container::flat_set<std::string>;
  std::vector<ProductSet> to_print;
  for (const Product& product : products) {
    ProductSet ids;
    for (const Literal& literal : product) {
      ids.insert((literal.complement ? "~" : "") + literal.event.name());
    }
    to_print.push_back(std::move(ids));
  }
  boost::sort(to_print, [](const ProductSet& lhs, const ProductSet& rhs) {
    if (lhs.size() == rhs.size())
      return lhs < rhs;
    return lhs.size() < rhs.size();
  });
  assert(!to_print.front().empty() && "Failure of the analysis with Unity!");

  for (const auto& product : to_print) {
    for (const auto& id : product)
      std::cerr << " " << id;
    std::cerr << "\n";
  }
  std::cerr << std::flush;
}

double Product::p() const {
  double p = 1;
  for (const Literal& literal : *this) {
    p *= literal.complement ? 1 - literal.event.p() : literal.event.p();
  }
  return p;
}

std::vector<int> ProductContainer::Distribution() const {
  std::vector<int> distribution;
  for (const std::vector<int>& product : products_) {
    int index = product.empty() ? 0 : product.size() - 1;
    if (distribution.size() <= index)
      distribution.resize(index + 1);
    distribution[index]++;
  }
  return distribution;
}

FaultTreeAnalysis::FaultTreeAnalysis(const mef::Gate& root,
                                     const Settings& settings)
    : Analysis(settings),
      top_event_(root) {}

void FaultTreeAnalysis::Analyze() noexcept {
  CLOCK(analysis_time);
  graph_ = std::make_unique<Pdag>(top_event_,
                                  Analysis::settings().ccf_analysis());
  this->Preprocess(graph_.get());
#ifndef NDEBUG
  if (Analysis::settings().preprocessor)
    return;  // Preprocessor only option.
#endif
  CLOCK(algo_time);
  LOG(DEBUG2) << "Launching the algorithm...";
  const Zbdd& products = this->GenerateProducts(graph_.get());
  LOG(DEBUG2) << "The algorithm finished in " << DUR(algo_time);
  LOG(DEBUG2) << "# of products: " << products.size();

  Analysis::AddAnalysisTime(DUR(analysis_time));
  CLOCK(store_time);
  Store(products, *graph_);
  LOG(DEBUG2) << "Stored the result for reporting in " << DUR(store_time);
}

void FaultTreeAnalysis::Store(const Zbdd& products,
                              const Pdag& graph) noexcept {
  // Special cases of sets.
  if (products.empty()) {
    Analysis::AddWarning("The set is NULL/Empty.");
  } else if (products.base()) {
    Analysis::AddWarning("The set is UNITY/Base.");
  }
  products_ = std::make_unique<const ProductContainer>(products, graph);

#ifndef NDEBUG
  for (const Product& product : *products_)
    assert(product.size() <= Analysis::settings().limit_order() &&
           "Miscalculated product sets with larger-than-required order.");

  if (Analysis::settings().print)
    Print(*products_);
#endif
}

}  // namespace core
}  // namespace scram
