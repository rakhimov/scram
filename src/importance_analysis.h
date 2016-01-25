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

/// @file importance_analysis.h
/// Contains functionality to do numerical analysis
/// of importance factors.

#ifndef SCRAM_SRC_IMPORTANCE_ANALYSIS_H_
#define SCRAM_SRC_IMPORTANCE_ANALYSIS_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bdd.h"
#include "event.h"
#include "probability_analysis.h"
#include "settings.h"

namespace scram {

/// @struct ImportanceFactors
/// Collection of importance factors for variables.
struct ImportanceFactors {
  double mif;  ///< Birnbaum marginal importance factor.
  double cif;  ///< Critical importance factor.
  double dif;  ///< Fussel-Vesely diagnosis importance factor.
  double raw;  ///< Risk achievement worth factor.
  double rrw;  ///< Risk reduction worth factor.
};

/// @class ImportanceAnalysis
/// Analysis of importance factors of risk model variables.
class ImportanceAnalysis : public Analysis {
 public:
  /// Importance analysis
  /// on the fault tree represented by
  /// its probability analysis.
  ///
  /// @param[in] prob_analysis  Completed probability analysis.
  explicit ImportanceAnalysis(const ProbabilityAnalysis* prob_analysis);

  virtual ~ImportanceAnalysis() = default;

  /// Performs quantitative analysis of importance factors
  /// of basic events in products.
  ///
  /// @pre Analysis is called only once.
  void Analyze() noexcept;

  /// @returns Map with basic events and their importance factors.
  ///
  /// @pre The importance analysis is done.
  const std::unordered_map<std::string, ImportanceFactors>& importance() const {
    return importance_;
  }

  /// @returns A collection of important events and their importance factors.
  ///
  /// @pre The importance analysis is done.
  const std::vector<std::pair<BasicEventPtr, ImportanceFactors>>&
  important_events() const {
    return important_events_;
  }

 protected:
  /// Gathers all events present in products.
  /// Only this events can have importance factors.
  ///
  /// @param[in] graph  Boolean graph with basic event indices and pointers.
  /// @param[in] products  Products with basic event indices.
  ///
  /// @returns A unique collection of important basic events.
  std::vector<std::pair<int, BasicEventPtr>> GatherImportantEvents(
      const BooleanGraph* graph,
      const std::vector<std::vector<int>>& products) noexcept;

 private:
  /// Find all events that are in the products.
  ///
  /// @returns Indices and pointers to the basic events.
  virtual std::vector<std::pair<int, BasicEventPtr>>
  GatherImportantEvents() noexcept = 0;

  /// Calculates Marginal Importance Factor.
  ///
  /// @param[in] index  Positive index of an event.
  ///
  /// @returns Calculated value for MIF.
  virtual double CalculateMif(int index) noexcept = 0;

  /// @returns Total probability from the probability analysis.
  virtual double p_total() noexcept = 0;

  /// Container for basic event importance factors.
  std::unordered_map<std::string, ImportanceFactors> importance_;
  /// Container of pointers to important events and their importance factors.
  std::vector<std::pair<BasicEventPtr, ImportanceFactors>> important_events_;
};

/// @class ImportanceAnalyzerBase
/// Analyzer of importance factors
/// with the help from probability analyzers.
///
/// @tparam Calculator  Quantitative calculator of probability values.
template<typename Calculator>
class ImportanceAnalyzerBase : public ImportanceAnalysis {
 public:
  /// Constructs importance analyzer from probability analyzer.
  /// Probability analyzer facilities are used
  /// to calculate the total and conditional probabilities for factors.
  ///
  /// @param[in] prob_analyzer  Instantiated probability analyzer.
  ///
  /// @pre Probability analyzer can work with modified probability values.
  ///
  /// @post Probability analyzer's probability values are
  ///       reset to the original values (event probabilities).
  explicit ImportanceAnalyzerBase(
      ProbabilityAnalyzer<Calculator>* prob_analyzer)
      : ImportanceAnalysis::ImportanceAnalysis(prob_analyzer),
        prob_analyzer_(prob_analyzer) {}

  virtual ~ImportanceAnalyzerBase() = 0;  ///< Abstract class.

  /// Find all events that are in the products.
  ///
  /// @returns Indices and pointers to the basic events.
  std::vector<std::pair<int, BasicEventPtr>> GatherImportantEvents() noexcept {
    return ImportanceAnalysis::GatherImportantEvents(
        prob_analyzer_->graph(),
        prob_analyzer_->products());
  }

  /// @returns Total probability calculated by probability analyzer.
  double p_total() noexcept { return prob_analyzer_->p_total(); }

 protected:
  /// Calculator of the total probability.
  ProbabilityAnalyzer<Calculator>* prob_analyzer_;
};

template<typename Calculator>
ImportanceAnalyzerBase<Calculator>::~ImportanceAnalyzerBase() {}

/// @class ImportanceAnalyzer
/// Analyzer of importance factors
/// with the help from probability analyzers.
///
/// @tparam Calculator  Quantitative calculator of probability values.
template<typename Calculator>
class ImportanceAnalyzer : public ImportanceAnalyzerBase<Calculator> {
 public:
  using ImportanceAnalyzerBase<Calculator>::ImportanceAnalyzerBase;

  double CalculateMif(int index) noexcept;

 private:
  using ImportanceAnalyzerBase<Calculator>::prob_analyzer_;
};

template<typename Calculator>
double ImportanceAnalyzer<Calculator>::CalculateMif(int index) noexcept {
  std::vector<double>& p_vars = prob_analyzer_->p_vars();
  // Calculate P(top/event)
  p_vars[index] = 1;
  double p_e = prob_analyzer_->CalculateTotalProbability();
  assert(p_e >= 0);
  if (p_e > 1) p_e = 1;

  // Calculate P(top/Not event)
  p_vars[index] = 0;
  double p_not_e = prob_analyzer_->CalculateTotalProbability();
  assert(p_not_e >= 0);
  if (p_not_e > 1) p_not_e = 1;

  // Restore the probability.
  p_vars[index] = prob_analyzer_->graph()->GetBasicEvent(index)->p();
  return p_e - p_not_e;
}

/// @class ImportanceAnalyzer<Bdd>
/// Specialization of importance analyzer with Binary Decision Diagrams.
template<>
class ImportanceAnalyzer<Bdd> : public ImportanceAnalyzerBase<Bdd> {
 public:
  /// Constructs importance analyzer from probability analyzer.
  /// Probability analyzer facilities are used
  /// to calculate the total and conditional probabilities for factors.
  ///
  /// @param[in] prob_analyzer  Instantiated probability analyzer.
  explicit ImportanceAnalyzer(ProbabilityAnalyzer<Bdd>* prob_analyzer)
      : ImportanceAnalyzerBase<Bdd>::ImportanceAnalyzerBase(prob_analyzer),
        bdd_graph_(prob_analyzer->bdd_graph()) {}

  double CalculateMif(int index) noexcept;

 private:
  using ImportanceAnalyzerBase<Bdd>::prob_analyzer_;

  /// Calculates Marginal Importance Factor of a variable.
  ///
  /// @param[in] vertex  The root vertex of a function graph.
  /// @param[in] order  The identifying order of the variable.
  /// @param[in] mark  A flag to mark traversed vertices.
  ///
  /// @returns Importance factor value.
  ///
  /// @note Probability factor fields are used to save results.
  /// @note The graph needs cleaning its marks after this function
  ///       because the graph gets continuously-but-partially marked.
  double CalculateMif(const VertexPtr& vertex, int order, bool mark) noexcept;

  /// Retrieves memorized probability values for BDD function graphs.
  ///
  /// @param[in] vertex  Vertex with calculated probabilities.
  ///
  /// @returns Saved probability value of the vertex.
  double RetrieveProbability(const VertexPtr& vertex) noexcept;

  Bdd* bdd_graph_;  ///< Binary decision diagram for the analyzer.
};

}  // namespace scram

#endif  // SCRAM_SRC_IMPORTANCE_ANALYSIS_H_
