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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis.h"
#include "boolean_graph.h"
#include "event.h"
#include "logger.h"
#include "preprocessor.h"
#include "settings.h"

namespace scram {

/// @struct Literal
/// Event or its complement
/// that may appear in products.
struct Literal {
  bool complement;  ///< Indication of a complement event.
  std::shared_ptr<BasicEvent> event;  ///< The event in the product.
};

using Product = std::vector<Literal>;  ///< Collection of unique literals.

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
/// @param[in] Product  A set of literals.
///
/// @returns The order of the product.
///
/// @note An empty set is assumed to indicate the Base/Unity set.
int GetOrder(const Product& product);

/// @class FaultTreeDescriptor
/// Fault tree description gatherer.
/// General information about a fault tree
/// described by a gate as its root.
class FaultTreeDescriptor {
 public:
  /// Gathers all information about a fault tree with a root gate.
  ///
  /// @param[in] root  The root gate of a fault tree.
  ///
  /// @pre Gate marks must be clear.
  ///
  /// @warning If the fault tree structure is changed,
  ///          this description does not incorporate the changed structure.
  ///          Moreover, the data may get corrupted.
  explicit FaultTreeDescriptor(const GatePtr& root);

  virtual ~FaultTreeDescriptor() = default;

  /// @returns The top gate that is passed to the analysis.
  const GatePtr& top_event() const { return top_event_; }

  /// @returns The container of intermediate events.
  ///
  /// @warning If the fault tree has changed,
  ///          this is only a snapshot of the past
  const std::unordered_map<std::string, GatePtr>& inter_events() const {
    return inter_events_;
  }

  /// @returns The container of all basic events of this tree.
  ///
  /// @warning If the fault tree has changed,
  ///          this is only a snapshot of the past
  const std::unordered_map<std::string, BasicEventPtr>& basic_events() const {
    return basic_events_;
  }

  /// @returns Basic events that are in some CCF groups.
  ///
  /// @warning If the fault tree has changed,
  ///          this is only a snapshot of the past
  const std::unordered_map<std::string, BasicEventPtr>& ccf_events() const {
    return ccf_events_;
  }

  /// @returns The container of house events of the fault tree.
  ///
  /// @warning If the fault tree has changed,
  ///          this is only a snapshot of the past
  const std::unordered_map<std::string, HouseEventPtr>& house_events() const {
    return house_events_;
  }

 private:
  /// Gathers information about the correctly initialized fault tree.
  /// Databases for events are manipulated
  /// to best reflect the state and structure of the fault tree.
  /// This function must be called after validation.
  /// This function must be called before any analysis is performed
  /// because there would not be necessary information
  /// available for analysis like primary events of this fault tree.
  /// Moreover, all the nodes of this fault tree
  /// are expected to be defined fully and correctly.
  /// Gates are marked upon visit.
  /// The mark is checked to prevent revisiting.
  ///
  /// @param[in] gate  The gate to start traversal from.
  void GatherEvents(const GatePtr& gate) noexcept;

  /// Traverses formulas recursively to find all events.
  ///
  /// @param[in] formula  The formula to get events from.
  void GatherEvents(const FormulaPtr& formula) noexcept;

  /// Clears marks from gates that were traversed.
  /// Marks are set to empty strings.
  /// This is important
  /// because other code may assume that marks are empty.
  void ClearMarks() noexcept;

  GatePtr top_event_;  ///< Top event of this fault tree.

  /// Container for intermediate events.
  std::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for basic events.
  std::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for house events of the tree.
  std::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for basic events that are identified to be in some CCF group.
  /// These basic events are not necessarily in the same CCF group.
  std::unordered_map<std::string, BasicEventPtr> ccf_events_;
};

/// @class FaultTreeAnalysis
/// Fault tree analysis functionality.
/// The analysis must be done on
/// a validated and fully initialized fault trees.
/// After initialization of the analysis,
/// the fault tree under analysis should not change;
/// otherwise, the success of the analysis is not guaranteed,
/// and the results may be invalid.
/// After the requested analysis is done,
/// the fault tree can be changed without restrictions.
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
  /// @pre The gates' visit marks are clean.
  ///
  /// @note It is assumed that analysis is done only once.
  ///
  /// @warning If the fault tree structure is changed,
  ///          this analysis does not incorporate the changed structure.
  ///          Moreover, the analysis results may get corrupted.
  FaultTreeAnalysis(const GatePtr& root, const Settings& settings);

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
  const std::vector<BasicEventPtr>& product_events() const {
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

  std::vector<Product> products_;  ///< Container of analysis results.
  std::vector<BasicEventPtr> product_events_;  ///< Basic events in the sets.
};

/// @class FaultTreeAnalyzer
///
/// @tparam Algorithm  Fault tree analysis algorithm.
///
/// Fault tree analysis facility with specific algorithms.
/// This class is meant to be specialized by fault tree analysis algorithms.
template<typename Algorithm>
class FaultTreeAnalyzer : public FaultTreeAnalysis {
 public:
  /// Constructor with a fault tree and analysis settings.
  using FaultTreeAnalysis::FaultTreeAnalysis;

  /// Runs fault tree analysis with the given algorithm.
  void Analyze() noexcept;

  /// @returns Pointer to the analysis algorithm.
  const Algorithm* algorithm() const { return algorithm_.get(); }

  /// @returns Pointer to the analysis algorithm
  ///          for use by other analyses.
  Algorithm* algorithm() { return algorithm_.get(); }

  /// @returns Pointer to the Boolean graph representing the fault tree.
  const BooleanGraph* graph() const { return graph_.get(); }

 protected:
  std::unique_ptr<Algorithm> algorithm_;  ///< Analysis algorithm.
  std::unique_ptr<BooleanGraph> graph_;  ///< Boolean graph of the fault tree.
};

template<typename Algorithm>
void FaultTreeAnalyzer<Algorithm>::Analyze() noexcept {
  CLOCK(analysis_time);

  CLOCK(graph_creation);
  graph_ = std::unique_ptr<BooleanGraph>(
      new BooleanGraph(FaultTreeDescriptor::top_event(),
                       kSettings_.ccf_analysis()));
  LOG(DEBUG2) << "Boolean graph is created in " << DUR(graph_creation);

  CLOCK(prep_time);  // Overall preprocessing time.
  LOG(DEBUG2) << "Preprocessing...";
  Preprocessor* preprocessor = new CustomPreprocessor<Algorithm>(graph_.get());
  preprocessor->Run();
  delete preprocessor;  // No exceptions are expected.
  LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);
#ifndef NDEBUG
  if (kSettings_.preprocessor) return;  // Preprocessor only option.
#endif
  CLOCK(algo_time);
  LOG(DEBUG2) << "Launching the algorithm...";
  algorithm_ =
      std::unique_ptr<Algorithm>(new Algorithm(graph_.get(), kSettings_));
  algorithm_->Analyze();
  LOG(DEBUG2) << "The algorithm finished in " << DUR(algo_time);

  analysis_time_ = DUR(analysis_time);  // Duration of MCS generation.
  FaultTreeAnalysis::Convert(algorithm_->products(), graph_.get());
}

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
