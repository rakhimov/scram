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

/// @file probability_analysis.h
/// Contains functionality to do numerical analysis of probabilities.

#ifndef SCRAM_SRC_PROBABILITY_ANALYSIS_H_
#define SCRAM_SRC_PROBABILITY_ANALYSIS_H_

#include <vector>

#include "analysis.h"
#include "bdd.h"
#include "fault_tree_analysis.h"
#include "pdag.h"

namespace scram {
namespace core {

/// Main quantitative analysis class.
class ProbabilityAnalysis : public Analysis {
 public:
  /// Probability analysis
  /// with the results of qualitative analysis.
  ///
  /// @param[in] fta  Fault tree analysis with results.
  ///
  /// @pre The underlying fault tree must not have changed in any way
  ///      since the fault tree analysis finished.
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
  /// @returns The total probability of the graph or products.
  virtual double CalculateTotalProbability() noexcept = 0;

  double p_total_;  ///< Total probability of the top event.
};

/// Quantitative calculator of a probability value of a single cut set.
class CutSetProbabilityCalculator {
 public:
  using CutSet = std::vector<int>;  ///< Alias for clarity.

  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set  A cut set with positive indices of basic events.
  /// @param[in] p_vars  Probabilities of events mapped by the variable indices.
  ///
  /// @returns The total probability of the cut set.
  ///
  /// @pre The cut set doesn't contain complements.
  /// @pre Probability values are non-negative.
  /// @pre Indices of events directly map to vector indices.
  double Calculate(const CutSet& cut_set,
                   const Pdag::IndexMap<double>& p_vars) noexcept;

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

/// Quantitative calculator of probability values
/// with the Rare-Event approximation.
class RareEventCalculator : private CutSetProbabilityCalculator {
 public:
  /// Calculates probabilities
  /// using the Rare-Event approximation.
  ///
  /// @param[in] cut_sets  A collection of sets of indices of basic events.
  /// @param[in] p_vars  Probabilities of events mapped by the variable indices.
  ///
  /// @returns The total probability with the rare-event approximation.
  ///
  /// @post In case the calculated probability exceeds 1,
  ///       the probability is adjusted to 1.
  ///       It is very unwise to use the rare-event approximation
  ///       with large probability values.
  double Calculate(const std::vector<CutSet>& cut_sets,
                   const Pdag::IndexMap<double>& p_vars) noexcept;
};

/// Quantitative calculator of probability values
/// with the Min-Cut-Upper Bound approximation.
class McubCalculator : private CutSetProbabilityCalculator {
 public:
  /// Calculates probabilities
  /// using the minimal cut set upper bound (MCUB) approximation.
  ///
  /// @param[in] cut_sets  A collection of sets of indices of basic events.
  /// @param[in] p_vars  Probabilities of events mapped by the variable indices.
  ///
  /// @returns The total probability with the MCUB approximation.
  double Calculate(const std::vector<CutSet>& cut_sets,
                   const Pdag::IndexMap<double>& p_vars) noexcept;
};

/// Base class for Probability analyzers.
class ProbabilityAnalyzerBase : public ProbabilityAnalysis {
 public:
  using Product = std::vector<int>;  ///< Alias for clarity.

  /// Constructs probability analyzer from a fault tree analyzer.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  template <class Algorithm>
  explicit ProbabilityAnalyzerBase(const FaultTreeAnalyzer<Algorithm>* fta)
      : ProbabilityAnalysis(fta),
        graph_(fta->graph()),
        products_(fta->algorithm()->products()) {
    ExtractVariableProbabilities();
  }

  /// @returns The original PDAG from the fault tree analyzer.
  const Pdag* graph() const { return graph_; }

  /// @returns The resulting products of the fault tree analyzer.
  const std::vector<Product>& products() const { return products_; }

  /// @returns A mapping for probability values with indices.
  const Pdag::IndexMap<double>& p_vars() const { return p_vars_; }

 protected:
  ~ProbabilityAnalyzerBase() = default;

 private:
  /// Upon construction of the probability analysis,
  /// stores the variable probabilities in a continuous container
  /// for retrieval by their indices instead of pointers.
  ///
  /// @note This function may seem redundant,
  ///       for it's super-short and simple to do it inline in the constructor.
  ///       The main benefit of the out-of-line implementation
  ///       is compile-time decoupling from the input BasicEvent classes.
  void ExtractVariableProbabilities();

  const Pdag* graph_;  ///< PDAG from the fault tree analysis.
  const std::vector<Product>& products_;  ///< A collection of products.
  Pdag::IndexMap<double> p_vars_;  ///< Variable probabilities.
};

/// Fault-tree-analysis-aware probability analyzer.
/// Probability analyzer provides the main engine for probability analysis.
///
/// @tparam Calculator  Quantitative analysis calculator.
template <class Calculator>
class ProbabilityAnalyzer : public ProbabilityAnalyzerBase {
 public:
  using ProbabilityAnalyzerBase::ProbabilityAnalyzerBase;

  /// Calculates the total probability
  /// with a different set of probability values
  /// than the one given upon construction.
  ///
  /// @param[in] p_vars  A map of probabilities of the graph variables.
  ///                    The indices of the variables must map
  ///                    exactly to the values.
  ///
  /// @returns The total probability calculated with the given values.
  double CalculateTotalProbability(
      const Pdag::IndexMap<double>& p_vars) noexcept {
    return calc_.Calculate(ProbabilityAnalyzerBase::products(), p_vars);
  }

 private:
  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or the sum of products.
  double CalculateTotalProbability() noexcept override {
    return CalculateTotalProbability(ProbabilityAnalyzerBase::p_vars());
  }

  Calculator calc_;  ///< Provider of the calculation logic.
};

/// Specialization of probability analyzer with Binary Decision Diagrams.
/// The quantitative analysis is done with BDD.
template <>
class ProbabilityAnalyzer<Bdd> : public ProbabilityAnalyzerBase {
 public:
  /// Constructs probability analyzer from a fault tree analyzer
  /// with the same algorithm.
  ///
  /// @tparam Algorithm  Fault tree analysis algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  template <class Algorithm>
  explicit ProbabilityAnalyzer(const FaultTreeAnalyzer<Algorithm>* fta)
      : ProbabilityAnalyzerBase(fta),
        current_mark_(false),
        owner_(true) {
    CreateBdd(*fta);
  }

  /// Reuses BDD structures from Fault tree analyzer.
  ///
  /// @param[in] fta  Finished fault tree analyzer with BDD algorithms.
  ///
  /// @pre BDD is fully formed and used.
  ///
  /// @post FaultTreeAnalyzer is not corrupted
  ///       by use of its BDD internals.
  explicit ProbabilityAnalyzer(FaultTreeAnalyzer<Bdd>* fta);

  /// Deletes the PDAG and BDD
  /// only if ProbabilityAnalyzer is the owner of them.
  ~ProbabilityAnalyzer() noexcept;

  /// @returns Binary decision diagram used for calculations.
  Bdd* bdd_graph() { return bdd_graph_; }

  /// @copydoc ProbabilityAnalyzer::CalculateTotalProbability
  double CalculateTotalProbability(
      const Pdag::IndexMap<double>& p_vars) noexcept;

 private:
  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph.
  double CalculateTotalProbability() noexcept override {
    return CalculateTotalProbability(ProbabilityAnalyzerBase::p_vars());
  }

  /// Creates a new BDD for use by the analyzer.
  ///
  /// @param[in] fta  The fault tree analysis providing the root gate.
  ///
  /// @pre The function is called in the constructor only once.
  void CreateBdd(const FaultTreeAnalysis& fta) noexcept;

  /// Calculates exact probability
  /// of a function graph represented by its root BDD vertex.
  ///
  /// @param[in] vertex  The root vertex of a function graph.
  /// @param[in] mark  A flag to mark traversed vertices.
  /// @param[in] p_vars  The probabilities of the variables
  ///                    mapped by their indices.
  ///
  /// @returns Probability value.
  ///
  /// @warning If a vertex is already marked with the input mark,
  ///          it will not be traversed and updated with a probability value.
  double CalculateProbability(const Bdd::VertexPtr& vertex, bool mark,
                              const Pdag::IndexMap<double>& p_vars) noexcept;

  Bdd* bdd_graph_;  ///< The main BDD graph for analysis.
  bool current_mark_;  ///< To keep track of BDD current mark.
  bool owner_;  ///< Indication that pointers are handles.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_PROBABILITY_ANALYSIS_H_
