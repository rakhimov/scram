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

/// @file importance_analysis.h
/// Contains functionality to do numerical analysis
/// of importance factors.

#ifndef SCRAM_SRC_IMPORTANCE_ANALYSIS_H_
#define SCRAM_SRC_IMPORTANCE_ANALYSIS_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

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
class ImportanceAnalysis : public ProbabilityAnalysis {
 public:
  /// Importance analysis
  /// on the fault tree represented by the root gate
  /// with Binary decision diagrams.
  ///
  /// @param[in] root The top event of the fault tree.
  /// @param[in] settings Analysis settings for probability calculations.
  ///
  /// @todo Remove this constructor.
  ImportanceAnalysis(const GatePtr& root, const Settings& settings);

  /// Performs quantitative analysis on minimal cut sets
  /// containing basic events provided in the databases.
  /// It is assumed that the analysis is called only once.
  ///
  /// @param[in] min_cut_sets Minimal cut sets with string ids of events.
  ///                         Negative event is indicated by "'not' + id"
  ///
  /// @note  Undefined behavior if analysis is called two or more times.
  virtual void Analyze(
      const std::set<std::set<std::string>>& min_cut_sets) noexcept;

  /// @returns Map with basic events and their importance factors.
  ///
  /// @note The user should make sure that the analysis is actually done.
  const std::unordered_map<std::string, ImportanceFactors>& importance() const {
    return importance_;
  }

 private:
  /// Importance analysis of basic events that are in minimal cut sets.
  void PerformImportanceAnalysis() noexcept;

  /// Performs BDD-based importance analysis.
  void PerformImportanceAnalysisBdd() noexcept;

  /// Calculates Marginal Importance Factor of a variable.
  ///
  /// @param[in] vertex The root vertex of a function graph.
  /// @param[in] order The identifying order of the variable.
  /// @param[in] mark A flag to mark traversed vertices.
  ///
  /// @note Probability fields are used to save results.
  /// @note The graph needs cleaning its marks after this function
  ///       because the graph gets continuously-but-partially marked.
  double CalculateMif(const VertexPtr& vertex, int order, bool mark) noexcept;

  /// Retrieves memorized probability values for BDD function graphs.
  ///
  /// @param[in] vertex Vertex with calculated probabilities.
  ///
  /// @returns Saved probability of the vertex.
  double RetrieveProbability(const VertexPtr& vertex) noexcept;

  /// Clears marks of vertices in BDD graph.
  ///
  /// @param[in] vertex The starting root vertex of the graph.
  /// @param[in] mark The desired mark for the vertices.
  ///
  /// @note Marks will propagate to modules as well.
  void ClearMarks(const VertexPtr& vertex, bool mark) noexcept;

  /// Container for basic event importance factors.
  std::unordered_map<std::string, ImportanceFactors> importance_;
};

}  // namespace scram

#endif  // SCRAM_SRC_IMPORTANCE_ANALYSIS_H_
