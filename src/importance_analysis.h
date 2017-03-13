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

/// @file importance_analysis.h
/// Contains functionality to do numerical analysis
/// of importance factors.

#ifndef SCRAM_SRC_IMPORTANCE_ANALYSIS_H_
#define SCRAM_SRC_IMPORTANCE_ANALYSIS_H_

#include <vector>

#include "bdd.h"
#include "probability_analysis.h"
#include "settings.h"

namespace scram {

namespace mef {  // Decouple from the analysis code header.
class BasicEvent;
}  // namespace mef

namespace core {

/// Collection of importance factors for variables.
struct ImportanceFactors {
  int occurrence;  ///< The number of products the variable is present in.
  double mif;  ///< Birnbaum marginal importance factor.
  double cif;  ///< Critical importance factor.
  double dif;  ///< Fussel-Vesely diagnosis importance factor.
  double raw;  ///< Risk achievement worth factor.
  double rrw;  ///< Risk reduction worth factor.
};

/// Mapping of an event and its importance.
struct ImportanceRecord {
  const mef::BasicEvent& event;  ///< The event occurring in products.
  const ImportanceFactors factors;  ///< The importance factors of the event.
};

class Zbdd;  // The container of products to be queries for important events.

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

  /// @returns A collection of important events and their importance factors.
  ///
  /// @pre The importance analysis is done.
  const std::vector<ImportanceRecord>& importance() const {
    return importance_;
  }

 private:
  /// @returns Total probability from the probability analysis.
  virtual double p_total() noexcept = 0;
  /// @returns All basic event candidates for importance calculations.
  virtual const std::vector<const mef::BasicEvent*>&
  basic_events() noexcept = 0;
  /// @returns Occurrences of basic events in products.
  virtual std::vector<int> occurrences() noexcept = 0;

  /// Calculates Marginal Importance Factor.
  ///
  /// @param[in] index  The position index of an event in events vector.
  ///
  /// @returns Calculated value for MIF.
  virtual double CalculateMif(int index) noexcept = 0;

  /// Container of important events and their importance factors.
  std::vector<ImportanceRecord> importance_;
};

/// Base class for analyzers of importance factors
/// with the help from probability analyzers.
class ImportanceAnalyzerBase : public ImportanceAnalysis {
 public:
  /// Constructs importance analyzer from probability analyzer.
  /// Probability analyzer facilities are used
  /// to calculate the total and conditional probabilities for factors.
  ///
  /// @param[in] prob_analyzer  Instantiated probability analyzer.
  explicit ImportanceAnalyzerBase(ProbabilityAnalyzerBase* prob_analyzer)
      : ImportanceAnalysis(prob_analyzer), prob_analyzer_(prob_analyzer) {}

 protected:
  virtual ~ImportanceAnalyzerBase() = default;

  /// @returns A pointer to the helper probability analyzer.
  ProbabilityAnalyzerBase* prob_analyzer() { return prob_analyzer_; }

 private:
  double p_total() noexcept override { return prob_analyzer_->p_total(); }
  const std::vector<const mef::BasicEvent*>& basic_events() noexcept override {
    return prob_analyzer_->graph()->basic_events();
  }
  std::vector<int> occurrences() noexcept override;

  /// Calculator of the total probability.
  ProbabilityAnalyzerBase* prob_analyzer_;
};

/// Analyzer of importance factors
/// with the help from probability analyzers.
///
/// @tparam Calculator  Quantitative calculator of probability values.
template <class Calculator>
class ImportanceAnalyzer : public ImportanceAnalyzerBase {
 public:
  /// @copydoc ImportanceAnalyzerBase::ImportanceAnalyzerBase
  explicit ImportanceAnalyzer(ProbabilityAnalyzer<Calculator>* prob_analyzer)
      : ImportanceAnalyzerBase(prob_analyzer),
        p_vars_(prob_analyzer->p_vars()) {}

 private:
  double CalculateMif(int index) noexcept override;
  Pdag::IndexMap<double> p_vars_;  ///< A copy of variable probabilities.
};

template <class Calculator>
double ImportanceAnalyzer<Calculator>::CalculateMif(int index) noexcept {
  index += Pdag::kVariableStartIndex;
  auto p_conditional = [index, this](bool state) {
    p_vars_[index] = state;
    return static_cast<ProbabilityAnalyzer<Calculator>*>(prob_analyzer())
        ->CalculateTotalProbability(p_vars_);
  };
  double p_store = p_vars_[index];  // Save the original value for restoring.
  double mif = p_conditional(true) - p_conditional(false);
  p_vars_[index] = p_store;  // Restore the probability for next calculation.
  return mif;
}

/// Specialization of importance analyzer with Binary Decision Diagrams.
template <>
class ImportanceAnalyzer<Bdd> : public ImportanceAnalyzerBase {
 public:
  /// Constructs importance analyzer from probability analyzer.
  /// Probability analyzer facilities are used
  /// to calculate the total and conditional probabilities for factors.
  ///
  /// @param[in] prob_analyzer  Instantiated probability analyzer.
  explicit ImportanceAnalyzer(ProbabilityAnalyzer<Bdd>* prob_analyzer)
      : ImportanceAnalyzerBase(prob_analyzer),
        bdd_graph_(prob_analyzer->bdd_graph()) {}

 private:
  double CalculateMif(int index) noexcept override;

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
  double CalculateMif(const Bdd::VertexPtr& vertex, int order,
                      bool mark) noexcept;

  /// Retrieves memorized probability values for BDD function graphs.
  ///
  /// @param[in] vertex  Vertex with calculated probabilities.
  ///
  /// @returns Saved probability value of the vertex.
  double RetrieveProbability(const Bdd::VertexPtr& vertex) noexcept;

  Bdd* bdd_graph_;  ///< Binary decision diagram for the analyzer.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_IMPORTANCE_ANALYSIS_H_
