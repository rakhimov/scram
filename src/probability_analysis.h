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

/// @file probability_analysis.h
/// Contains functionality to do numerical analysis of probabilities.

#ifndef SCRAM_SRC_PROBABILITY_ANALYSIS_H_
#define SCRAM_SRC_PROBABILITY_ANALYSIS_H_

#include <vector>

#include "analysis.h"
#include "bdd.h"
#include "boolean_graph.h"
#include "event.h"
#include "fault_tree_analysis.h"
#include "logger.h"
#include "settings.h"

namespace scram {

/// @class ProbabilityAnalysis
/// Main quantitative analysis class.
class ProbabilityAnalysis : public Analysis {
 public:
  /// Probability analysis
  /// with the results of qualitative analysis.
  ///
  /// @param[in] fta  Fault tree analysis with results.
  explicit ProbabilityAnalysis(const FaultTreeAnalysis* fta);

  virtual ~ProbabilityAnalysis() = default;

  /// Performs quantitative analysis on the supplied fault tree.
  ///
  /// @pre Analysis is called only once.
  void Analyze() noexcept;

  /// @returns The total probability calculated by the analysis.
  ///
  /// @note The user should make sure that the analysis is actually done.
  double p_total() const { return p_total_; }

 private:
  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or cut sets.
  virtual double CalculateTotalProbability() noexcept = 0;

  double p_total_;  ///< Total probability of the top event.
};

/// @class CutSetProbabilityCalculator
/// Quantitative calculator of a probability value of a single cut set.
class CutSetProbabilityCalculator {
 public:
  using CutSet = std::vector<int>;

  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set  A cut set with indices of basic events.
  /// @param[in] var_probs  Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability of the cut set.
  ///
  /// @pre The cut set doesn't contain complements.
  /// @pre Probability values are non-negative.
  /// @pre Indices of events directly map to vector indices.
  double Calculate(const CutSet& cut_set,
                   const std::vector<double>& var_probs) noexcept;

  /// Checks the special case of a unity set with probability 1.
  ///
  /// @param[in] cut_sets  Collection of ALL cut sets.
  ///
  /// @returns true if the Unity set is detected.
  ///
  /// @pre The unity set is indicated by a single empty set.
  /// @pre Provided cut sets are ALL the cut sets under consideration.
  double CheckUnity(const std::vector<CutSet>& cut_sets) noexcept {
    return cut_sets.size() == 1 && cut_sets.front().empty();
  }
};

/// @class RareEventCalculator
/// Quantitative calculator of probability values
/// with the Rare-Event approximation.
class RareEventCalculator : private CutSetProbabilityCalculator {
 public:
  using CutSetProbabilityCalculator::CutSet;

  /// Calculates probabilities
  /// using the Rare-Event approximation.
  ///
  /// @param[in] cut_sets  A collection of sets of indices of basic events.
  /// @param[in] var_probs  Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability with the rare-event approximation.
  ///
  /// @pre Absolute indices of events directly map to vector indices.
  ///
  /// @post The returned probability value may not be acceptable.
  ///       That is, it may be out of the acceptable [0, 1] range.
  ///       The caller of this function must decide
  ///       what to do in this case.
  double Calculate(const std::vector<CutSet>& cut_sets,
                   const std::vector<double>& var_probs) noexcept;
};

/// @class McubCalculator
/// Quantitative calculator of probability values
/// with the Min-Cut-Upper Bound approximation.
class McubCalculator : private CutSetProbabilityCalculator {
 public:
  using CutSetProbabilityCalculator::CutSet;

  /// Calculates probabilities
  /// using the minimal cut set upper bound (MCUB) approximation.
  ///
  /// @param[in] cut_sets  A collection of sets of indices of basic events.
  /// @param[in] var_probs  Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability with the MCUB approximation.
  double Calculate(const std::vector<CutSet>& cut_sets,
                   const std::vector<double>& var_probs) noexcept;
};

/// @class ProbabilityAnalyzerBase
/// Aggregation class for Probability analyzers.
class ProbabilityAnalyzerBase : public ProbabilityAnalysis {
 public:
  using CutSet = std::vector<int>;

  /// Constructs probability analyzer from a fault tree analyzer.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  template<typename Algorithm>
  explicit ProbabilityAnalyzerBase(const FaultTreeAnalyzer<Algorithm>* fta);

  /// @returns The original Boolean graph from the fault tree analyzer.
  const BooleanGraph* graph() const { return graph_; }

  /// @returns The resulting cut sets of the fault tree analyzer.
  const std::vector<CutSet>& cut_sets() const { return cut_sets_; }

  /// @returns A modifiable mapping for probability values and indices.
  ///
  /// @pre Quantitative analyzers aware of how Probability analyzer works.
  /// @pre Quantitative analyzers will cleanup after themselves.
  ///
  /// @warning This is a temporary hack
  ///          due to tight coupling of Quantitative analyzers.
  std::vector<double>& var_probs() { return var_probs_; }

 protected:
  const BooleanGraph* graph_;  ///< Boolean graph from the fault tree analysis.
  const std::vector<CutSet>& cut_sets_;  ///< A collection of cut sets.
  std::vector<double> var_probs_;  ///< Variable probabilities.
};

template<typename Algorithm>
ProbabilityAnalyzerBase::ProbabilityAnalyzerBase(
    const FaultTreeAnalyzer<Algorithm>* fta)
    : ProbabilityAnalysis::ProbabilityAnalysis(fta),
      graph_(fta->graph()),
      cut_sets_(fta->algorithm()->cut_sets()) {
  var_probs_.push_back(-1);  // Padding.
  for (const BasicEventPtr& event : graph_->basic_events()) {
    var_probs_.push_back(event->p());
  }
}

/// @class ProbabilityAnalyzer
/// Fault-tree-analysis-aware probability analyzer.
/// Probability analyzer provides the main engine for probability analysis.
///
/// @tparam Calculator  Quantitative analysis calculator.
template<typename Calculator>
class ProbabilityAnalyzer : public ProbabilityAnalyzerBase {
 public:
  using ProbabilityAnalyzerBase::ProbabilityAnalyzerBase;

  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or cut sets.
  double CalculateTotalProbability() noexcept {
    return calc_.Calculate(cut_sets_, var_probs_);
  }

 private:
  Calculator calc_;  ///< Provider of the calculation logic.
};

/// @class ProbabilityAnalyzer<Bdd>
/// Specialization of probability analyzer with Binary Decision Diagrams.
/// The quantitative analysis is done with BDD.
template<>
class ProbabilityAnalyzer<Bdd> : public ProbabilityAnalyzerBase {
 public:
  /// Constructs probability analyzer from a fault tree analyzer
  /// with the same algorithm.
  ///
  /// @tparam Algorithm  Fault tree analysis algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  template<typename Algorithm>
  explicit ProbabilityAnalyzer(const FaultTreeAnalyzer<Algorithm>* fta);

  /// Reuses BDD structures from Fault tree analyzer.
  ///
  /// @param[in] fta  Finished fault tree analyzer with BDD algorithms.
  ///
  /// @pre BDD is fully formed and used.
  ///
  /// @post FaultTreeAnalyzer is not corrupted
  ///       by use of its BDD internals.
  explicit ProbabilityAnalyzer(FaultTreeAnalyzer<Bdd>* fta);

  /// Deletes the Boolean graph and BDD
  /// only if ProbabilityAnalyzer is the owner of them.
  ~ProbabilityAnalyzer() noexcept;

  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or cut sets.
  double CalculateTotalProbability() noexcept;

  /// @returns Binary decision diagram used for calculations.
  Bdd* bdd_graph() { return bdd_graph_; }

 private:
  /// Creates a new BDD for use by the analyzer.
  ///
  /// @param[in] root  The root gate of the fault tree.
  ///
  /// @pre The function is called in the constructor only once.
  void CreateBdd(const GatePtr& root) noexcept;

  /// Calculates exact probability
  /// of a function graph represented by its root BDD vertex.
  ///
  /// @param[in] vertex  The root vertex of a function graph.
  /// @param[in] mark  A flag to mark traversed vertices.
  ///
  /// @returns Probability value.
  ///
  /// @warning If a vertex is already marked with the input mark,
  ///          it will not be traversed and updated with a probability value.
  double CalculateProbability(const VertexPtr& vertex, bool mark) noexcept;

  Bdd* bdd_graph_;  ///< The main BDD graph for analysis.
  bool current_mark_;  ///< To keep track of BDD current mark.
  bool owner_;  ///< Indication that pointers are handles.
};

template<typename Algorithm>
ProbabilityAnalyzer<Bdd>::ProbabilityAnalyzer(
    const FaultTreeAnalyzer<Algorithm>* fta)
    : ProbabilityAnalyzerBase::ProbabilityAnalyzerBase(fta),
      current_mark_(false),
      owner_(true) {
  CLOCK(main_time);
  ProbabilityAnalyzer::CreateBdd(fta->top_event());
  analysis_time_ = DUR(main_time);
}

}  // namespace scram

#endif  // SCRAM_SRC_PROBABILITY_ANALYSIS_H_
