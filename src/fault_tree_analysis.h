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

/// @file fault_tree_analysis.h
/// Fault Tree Analysis.

#ifndef SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
#define SCRAM_SRC_FAULT_TREE_ANALYSIS_H_

#include <cstdint>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "analysis.h"
#include "boolean_graph.h"
#include "event.h"
#include "ext.h"
#include "logger.h"
#include "preprocessor.h"
#include "settings.h"

namespace scram {
namespace core {

/// Event or its complement
/// that may appear in products.
struct Literal {
  bool complement;  ///< Indication of a complement event.
  const mef::BasicEvent& event;  ///< The event in the product.
};

/// Collection of unique literals.
/// This collection is designed as a space-efficient drop-in replacement
/// for const std::vector<const Literal>.
///
/// The major differences from std::vector:
///
///     1. Iterators are proxies.
///     2. The size is fixed upon construction.
///     3. The size is limited to 32 literals.
///     4. Only necessary and required API is provided for the use-cases.
///     5. The construction is handled differently.
class Product {
 public:
  /// Read iterator over Product Literals.
  class iterator {
    /// Equality test operators.
    /// @{
    friend bool operator==(const iterator& lhs, const iterator& rhs) {
      return &lhs.product_ == &rhs.product_ && lhs.pos_ == rhs.pos_;
    }
    friend bool operator!=(const iterator& lhs, const iterator& rhs) {
      return !(lhs == rhs);
    }
    /// @}

   public:
    /// Constructs an iterator on a position in a specific product.
    ///
    /// @param[in] pos  Valid position.
    /// @param[in] product  The host container.
    iterator(int pos, const Product& product) : pos_(pos), product_(product) {}

    /// @returns The literal at the position.
    Literal operator*() const {
      return {product_.GetComplement(pos_), *product_.data_[pos_]};
    }

    /// Pre-increment operator for for-range loops.
    iterator& operator++() {
      ++pos_;
      return *this;
    }

   private:
    int pos_;  ///< The current position of the iterator.
    const Product& product_;  ///< The container to be iterated over.
  };

  /// Initializes product literals.
  ///
  /// @tparam GeneratorIterator  Iterator type with its operator* returning
  ///                            std::pair or struct with
  ///                            ``first`` being complement flag of the Literal,
  ///                            ``second`` being pointer to the event.
  ///
  /// @param[in] size  The number of literals in the product.
  /// @param[in] it  The generator of literals for this product.
  template <class GeneratorIterator>
  Product(int size, GeneratorIterator it) noexcept
      : size_(size),
        complement_vector_(0),
        data_(new const mef::BasicEvent*[size]) {
    assert(size >= 0 && size <= 32);
    for (int i = 0; i < size; ++i, ++it) {
      const auto& literal = *it;
      if (literal.first) complement_vector_ |= 1 << i;  // The complement flag.
      data_[i] = literal.second;  // The variable itself.
    }
  }

  /// Move constructor for products to store in a database.
  ///
  /// @param[in] other  Fully initialized product.
  Product(Product&& other) noexcept
      : size_(other.size_),
        complement_vector_(other.complement_vector_),
        data_(std::move(other.data_)) {
    other.size_ = other.complement_vector_ = 0;
  }

  /// @returns true for unity product with no literals.
  bool empty() const { return size_ == 0; }

  /// @returns The number of literals in the product.
  int size() const { return size_; }

  /// @returns A read proxy iterator that points to the first element.
  iterator begin() const { return iterator(0, *this); }

  /// @returns A sentinel iterator signifying the end of iteration.
  iterator end() const { return iterator(size_, *this); }

 private:
  /// Determines if the literal is complement.
  ///
  /// @param[in] pos  The position of the literal.
  ///
  /// @returns true if the literal is complement.
  bool GetComplement(int pos) const {
    return complement_vector_ & (1 << pos);
  }

  uint8_t size_;  ///< The number of literals in the product.
  uint32_t complement_vector_;  ///< The complement flags of the literals.
  /// The collection of literal events.
  std::unique_ptr<const mef::BasicEvent*[]> data_;
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
void Print(const std::vector<Product>& products);

/// Helper function to compute a Boolean product probability.
///
/// @param[in] product  A set of literals.
///
/// @returns Product of probabilities of the literals.
///
/// @pre Events are initialized with expressions.
double CalculateProbability(const Product& product);

/// Helper function to determine order of a Boolean product.
///
/// @param[in] product  A set of literals.
///
/// @returns The order of the product.
///
/// @note An empty set is assumed to indicate the Base/Unity set.
int GetOrder(const Product& product);

/// Fault tree description gatherer.
/// General information about a fault tree
/// described by a gate as its root.
class FaultTreeDescriptor {
 public:
  /// Table of fault tree events with their ids as keys.
  template <typename T>
  using Table = std::unordered_map<std::string, const T*>;

  /// Gathers all information about a fault tree with a root gate.
  ///
  /// @param[in] root  The root gate of a fault tree.
  ///
  /// @warning If the fault tree structure is changed,
  ///          this description does not incorporate the changed structure.
  ///          Moreover, the data may get corrupted.
  explicit FaultTreeDescriptor(const mef::Gate& root);

  virtual ~FaultTreeDescriptor() = default;

  /// @returns The top gate that is passed to the analysis.
  const mef::Gate& top_event() const { return top_event_; }

  /// @returns The container of fault tree events.
  ///
  /// @warning If the fault tree has changed,
  ///          this is only a snapshot of the past
  /// @{
  const Table<mef::Gate>& inter_events() const { return inter_events_; }
  const Table<mef::BasicEvent>& basic_events() const { return basic_events_; }
  const Table<mef::BasicEvent>& ccf_events() const { return ccf_events_; }
  const Table<mef::HouseEvent>& house_events() const { return house_events_; }
  /// @}

 private:
  /// Traverses formulas recursively to find all events.
  /// Gathers information about the correctly initialized fault tree.
  /// Databases for events are manipulated
  /// to best reflect the state and structure of the fault tree.
  /// This function must be called after validation.
  /// This function must be called before any analysis is performed
  /// because there would not be necessary information
  /// available for analysis like primary events of this fault tree.
  /// Moreover, all the nodes of this fault tree
  /// are expected to be defined fully and correctly.
  ///
  /// @param[in] formula  The formula to get events from.
  void GatherEvents(const mef::Formula& formula) noexcept;

  const mef::Gate& top_event_;  ///< Top event of this fault tree.

  /// Containers of gathered fault tree events.
  /// @{
  Table<mef::Gate> inter_events_;
  Table<mef::BasicEvent> basic_events_;
  Table<mef::HouseEvent> house_events_;
  /// @}

  /// Container for basic events that are identified to be in some CCF group.
  /// These basic events are not necessarily in the same CCF group.
  Table<mef::BasicEvent> ccf_events_;
};

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
class FaultTreeAnalysis : public Analysis, public FaultTreeDescriptor {
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
  virtual void Analyze() noexcept = 0;

  /// @returns A set of Boolean products as the analysis results.
  const std::vector<Product>& products() const { return products_; }

  /// @returns Collection of basic events that are in the products.
  const std::unordered_set<const mef::BasicEvent*>& product_events() const {
    return product_events_;
  }

 protected:
  /// Converts resultant sets of basic event indices to strings
  /// for future reporting.
  /// This function also collects basic events in products.
  ///
  /// @param[in] results  A sets with indices of events from calculations.
  /// @param[in] graph  Boolean graph with basic event indices and pointers.
  void Convert(const std::vector<std::vector<int>>& results,
               const BooleanGraph* graph) noexcept;

 private:
  std::vector<Product> products_;  ///< Container of analysis results.
  /// The set of events in the resultant products.
  std::unordered_set<const mef::BasicEvent*> product_events_;
};

/// Fault tree analysis facility with specific algorithms.
/// This class is meant to be specialized by fault tree analysis algorithms.
///
/// @tparam Algorithm  Fault tree analysis algorithm.
template <class Algorithm>
class FaultTreeAnalyzer : public FaultTreeAnalysis {
 public:
  /// Constructor with a fault tree and analysis settings.
  using FaultTreeAnalysis::FaultTreeAnalysis;

  /// Runs fault tree analysis with the given algorithm.
  void Analyze() noexcept override;

  /// @returns Pointer to the analysis algorithm.
  const Algorithm* algorithm() const { return algorithm_.get(); }

  /// @returns Pointer to the analysis algorithm
  ///          for use by other analyses.
  Algorithm* algorithm() { return algorithm_.get(); }

  /// @returns Pointer to the Boolean graph representing the fault tree.
  const BooleanGraph* graph() const { return graph_.get(); }

 private:
  std::unique_ptr<Algorithm> algorithm_;  ///< Analysis algorithm.
  std::unique_ptr<BooleanGraph> graph_;  ///< Boolean graph of the fault tree.
};

template <class Algorithm>
void FaultTreeAnalyzer<Algorithm>::Analyze() noexcept {
  CLOCK(analysis_time);

  CLOCK(graph_creation);
  graph_ = ext::make_unique<BooleanGraph>(FaultTreeDescriptor::top_event(),
                                          Analysis::settings().ccf_analysis());
  LOG(DEBUG2) << "Boolean graph is created in " << DUR(graph_creation);

  CLOCK(prep_time);  // Overall preprocessing time.
  LOG(DEBUG2) << "Preprocessing...";
  Preprocessor* preprocessor = new CustomPreprocessor<Algorithm>(graph_.get());
  preprocessor->Run();
  delete preprocessor;  // No exceptions are expected.
  LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);
#ifndef NDEBUG
  if (Analysis::settings().preprocessor) return;  // Preprocessor only option.
#endif
  CLOCK(algo_time);
  LOG(DEBUG2) << "Launching the algorithm...";
  algorithm_ = ext::make_unique<Algorithm>(graph_.get(), Analysis::settings());
  algorithm_->Analyze();
  LOG(DEBUG2) << "The algorithm finished in " << DUR(algo_time);
  LOG(DEBUG2) << "# of products: " << algorithm_->products().size();

  Analysis::AddAnalysisTime(DUR(analysis_time));
  CLOCK(convert_time);
  FaultTreeAnalysis::Convert(algorithm_->products(), graph_.get());
  LOG(DEBUG2) << "Converted indices to pointers in " << DUR(convert_time);
}

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
