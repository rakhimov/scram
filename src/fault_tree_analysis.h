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

/// @file fault_tree_analysis.h
/// Fault Tree Analysis.

#ifndef SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
#define SCRAM_SRC_FAULT_TREE_ANALYSIS_H_

#include <cstdlib>

#include <memory>
#include <unordered_set>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "analysis.h"
#include "pdag.h"
#include "preprocessor.h"
#include "settings.h"
#include "zbdd.h"

namespace scram {

namespace mef {  // Decouple from the analysis code.
class Gate;
class BasicEvent;
}  // namespace mef

namespace core {

/// Event or its complement
/// that may appear in products.
struct Literal {
  bool complement;  ///< Indication of a complement event.
  const mef::BasicEvent& event;  ///< The event in the product.
};

/// Collection of unique literals.
class Product {
  /// Converter of indices in products to Literals.
  struct LiteralExtractor {
    /// @param[in] index  The index in a result product.
    ///
    /// @returns Literal representing the event with the index.
    Literal operator()(int index) const {
      return {index < 0, *graph.basic_events()[std::abs(index)]};
    }
    const Pdag& graph;  ///< The host graph.
  };

 public:
  /// Initializes product literals.
  ///
  /// @param[in] data  The underlying set.
  /// @param[in] graph  The graph with indices to events map.
  Product(const std::vector<int>& data, const Pdag& graph) noexcept
      : data_(data),
        graph_(graph) {}

  /// @returns true for unity product with no literals.
  bool empty() const { return data_.empty(); }

  /// @returns The number of literals in the product.
  int size() const { return data_.size(); }

  /// @returns The order of the product.
  ///
  /// @note An empty set indicates the Base/Unity set.
  int order() const { return empty() ? 1 : size(); }

  /// @returns The product of the literal probabilities.
  ///
  /// @pre Events are initialized with expressions.
  double p() const;

  /// @returns A read proxy iterator that points to the first element.
  auto begin() const {
    return boost::make_transform_iterator(data_.begin(),
                                          LiteralExtractor{graph_});
  }

  /// @returns A sentinel iterator signifying the end of iteration.
  auto end() const {
    return boost::make_transform_iterator(data_.end(),
                                          LiteralExtractor{graph_});
  }

 private:
  const std::vector<int>& data_;  ///< The collection of event indices.
  const Pdag& graph_;  ///< The host graph.
};

/// A container of analysis result products with Literals.
/// This is a wrapper of the analysis resultant ZBDD to work with Literals.
class ProductContainer {
  /// Converter of analysis products with indices into products with literals.
  struct ProductExtractor {
    /// @param[in] product  The product with indices from analysis results.
    ///
    /// @returns The wrapped product of Literals.
    Product operator()(const std::vector<int>& product) const {
      return Product(product, graph);
    }
    const Pdag& graph;  ///< The host graph.
  };

 public:
  /// The constructor also collects basic events in products.
  ///
  /// @param[in] products  Sets with indices of events from calculations.
  /// @param[in] graph  PDAG with basic event indices and pointers.
  ProductContainer(const Zbdd& products, const Pdag& graph) noexcept
      : products_(products),
        graph_(graph) {
    Pdag::IndexMap<bool> filter(graph_.basic_events().size());
    for (const std::vector<int>& result_set : products_) {
      for (int i : result_set) {
        i = std::abs(i);
        if (filter[i])
          continue;
        filter[i] = true;
        product_events_.insert(graph_.basic_events()[i]);
      }
    }
  }

  /// @returns Collection of basic events that are in the products.
  const std::unordered_set<const mef::BasicEvent*>& product_events() const {
    return product_events_;
  }

  /// Begin and end iterators over products in the container.
  /// @{
  auto begin() const {
    return boost::make_transform_iterator(products_.begin(),
                                          ProductExtractor{graph_});
  }
  auto end() const {
    return boost::make_transform_iterator(products_.end(),
                                          ProductExtractor{graph_});
  }
  /// @}

  /// @returns true if no products in the container.
  bool empty() const { return products_.empty(); }

  /// @returns The number of products in the container.
  int size() const { return products_.size(); }

  /// @returns The product distribution by order.
  std::vector<int> Distribution() const;

 private:
  const Zbdd& products_;  ///< Container of analysis results.
  const Pdag& graph_;  ///< The analysis graph.
  /// The set of events in the resultant products.
  std::unordered_set<const mef::BasicEvent*> product_events_;
};

/// Prints a collection of products to the standard error.
/// This is a helper function for easier debugging
/// and visual comparison of analysis results.
/// Summary of the number of products and
/// its size distribution is printed first.
/// Then, all products are grouped by size and sorted.
/// The literals of a product are sorted by their names.
///
/// @param[in] products  Valid, unique collection of analysis results.
void Print(const ProductContainer& products);

/// Fault tree analysis functionality.
/// The analysis must be done on
/// a validated and fully initialized fault trees.
/// After initialization of the analysis,
/// the fault tree under analysis should not change;
/// otherwise, the success of the analysis is not guaranteed,
/// and the results may be invalid.
/// After the requested analysis is done,
/// the fault tree can be changed without restrictions.
/// However, other analyses may rely on unchanged fault tree
/// to use the results of this fault tree analysis.
///
/// To conduct a new analysis on the changed fault tree,
/// a new FaultTreeAnalysis object must be created.
/// In general, rerunning the same analysis twice
/// will mess up the analysis and corrupt the previous results.
///
/// @warning Run analysis only once.
///          One analysis per FaultTreeAnalysis object.
class FaultTreeAnalysis : public Analysis {
 public:
  /// Traverses a valid fault tree from the root gate
  /// to collect databases of events, gates,
  /// and other members of the fault tree.
  /// The passed fault tree must be pre-validated without cycles,
  /// and its events must be fully initialized.
  ///
  /// @param[in] root  The top event of the fault tree to analyze.
  /// @param[in] settings  Analysis settings for all calculations.
  ///
  /// @note It is assumed that analysis is done only once.
  ///
  /// @warning If the fault tree structure is changed,
  ///          this analysis does not incorporate the changed structure.
  ///          Moreover, the analysis results may get corrupted.
  FaultTreeAnalysis(const mef::Gate& root, const Settings& settings);

  virtual ~FaultTreeAnalysis() = default;

  /// @returns The top gate that is passed to the analysis.
  const mef::Gate& top_event() const { return top_event_; }

  /// Analyzes the fault tree and performs computations.
  /// This function must be called
  /// only after initializing the fault tree
  /// with or without its probabilities.
  ///
  /// @note This function is expected to be called only once.
  ///
  /// @warning If the original fault tree is invalid,
  ///          this function will not throw or indicate any errors.
  ///          The behavior is undefined for invalid fault trees.
  /// @warning If the fault tree structure has changed
  ///          since the construction of the analysis,
  ///          the analysis will be invalid or fail.
  void Analyze() noexcept;

  /// @returns A collection of Boolean products as the analysis results.
  ///
  /// @pre The analysis is done.
  const ProductContainer& products() const {
    assert(products_ && "The analysis is not done!");
    return *products_;
  }

 protected:
  /// @returns Pointer to the PDAG representing the fault tree.
  const Pdag* graph() const { return graph_.get(); }

 private:
  /// Preprocesses a PDAG for future analysis with a specific algorithm.
  ///
  /// @param[in,out] graph  A valid PDAG for analysis.
  ///
  /// @post The graph transformation is semantically equivalent/isomorphic.
  virtual void Preprocess(Pdag* graph) noexcept = 0;

  /// Generates a sum of products from a preprocessed PDAG.
  ///
  /// @param[in] graph  The analysis PDAG.
  ///
  /// @returns The set of products.
  ///
  /// @pre The graph is specifically preprocessed for the algorithm.
  ///
  /// @post The result ZBDD lives as long as the host analysis.
  virtual const Zbdd& GenerateProducts(const Pdag* graph) noexcept = 0;

  /// Stores resultant sets of products for future reporting.
  ///
  /// @param[in] products  Sets with indices of events from calculations.
  /// @param[in] graph  PDAG with basic event indices and pointers.
  void Store(const Zbdd& products, const Pdag& graph) noexcept;

  const mef::Gate& top_event_;  ///< The root of the graph under analysis.
  std::unique_ptr<Pdag> graph_;  ///< PDAG of the fault tree.
  std::unique_ptr<const ProductContainer> products_;  ///< Container of results.
};

/// Fault tree analysis facility with specific algorithms.
/// This class is meant to be specialized by fault tree analysis algorithms.
///
/// @tparam Algorithm  Fault tree analysis algorithm.
template <class Algorithm>
class FaultTreeAnalyzer : public FaultTreeAnalysis {
 public:
  using FaultTreeAnalysis::FaultTreeAnalysis;
  using FaultTreeAnalysis::graph;  // Provide access to other analyses.

  /// @returns The analysis algorithm for use by other analyses.
  /// @{
  const Algorithm* algorithm() const { return algorithm_.get(); }
  Algorithm* algorithm() { return algorithm_.get(); }
  /// @}

 private:
  void Preprocess(Pdag* graph) noexcept override {
    CustomPreprocessor<Algorithm>{graph}();
  }

  const Zbdd& GenerateProducts(const Pdag* graph) noexcept override {
    algorithm_ = std::make_unique<Algorithm>(graph, Analysis::settings());
    algorithm_->Analyze();
    return algorithm_->products();
  }

  std::unique_ptr<Algorithm> algorithm_;  ///< Analysis algorithm.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
