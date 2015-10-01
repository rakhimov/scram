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
/// Contains functionality to do numerical analysis
/// of probabilities and importance factors.

#ifndef SCRAM_SRC_PROBABILITY_ANALYSIS_H_
#define SCRAM_SRC_PROBABILITY_ANALYSIS_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis.h"
#include "bdd.h"
#include "boolean_graph.h"
#include "event.h"
#include "fault_tree_analysis.h"
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

/// @class ProbabilityAnalysis
/// Main quantitative analysis.
class ProbabilityAnalysis : public Analysis {
 public:
  using BasicEventPtr = std::shared_ptr<BasicEvent>;
  using GatePtr = std::shared_ptr<Gate>;
  using VertexPtr = std::shared_ptr<Vertex>;
  using ItePtr = std::shared_ptr<Ite>;

  /// Probability analysis
  /// on the fault tree represented by the root gate
  /// with Binary decision diagrams.
  ///
  /// @param[in] root The top event of the fault tree.
  /// @param[in] settings Analysis settings for probability calculations.
  ///
  /// @note This technique does not require cut sets.
  ProbabilityAnalysis(const GatePtr& root, const Settings& settings);

  explicit ProbabilityAnalysis(const FaultTreeAnalysis* fta);

  virtual ~ProbabilityAnalysis() {}

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

  /// @returns The total probability calculated by the analysis.
  ///
  /// @note The user should make sure that the analysis is actually done.
  double p_total() const { return p_total_; }

  /// @returns Map with basic events and their importance factors.
  ///
  /// @note The user should make sure that the analysis is actually done.
  const std::unordered_map<std::string, ImportanceFactors>& importance() const {
    return importance_;
  }

  /// @returns Warnings generated upon analysis.
  const std::string warnings() const { return warnings_; }

  /// @returns The container of basic events of supplied for the analysis.
  const std::unordered_map<std::string, BasicEventPtr>& basic_events() const {
    return basic_events_;
  }

  /// @returns Analysis time spent on calculating the total probability.
  double prob_analysis_time() const { return p_time_; }

  /// @returns Analysis time spent on calculating the importance factors.
  double imp_analysis_time() const { return imp_time_; }

 protected:
  using CutSet = std::vector<int>;  ///< Unique positive or negative literals.

  /// Assigns an index to each basic event,
  /// and then populates with these indices
  /// new databases and basic-to-integer converting maps.
  /// The previous data are lost.
  /// These indices will be used for future analysis.
  void AssignIndices() noexcept;

  /// Populates databases of minimal cut sets
  /// with indices of the events.
  /// This traversal detects
  /// if cut sets contain complement events
  /// and turns non-coherent analysis.
  ///
  /// @param[in] min_cut_sets Minimal cut sets with event IDs.
  void IndexMcs(const std::set<std::set<std::string>>& min_cut_sets) noexcept;

  /// Calculates probabilities
  /// using the minimal cut set upper bound (MCUB) approximation.
  ///
  /// @param[in] min_cut_sets Sets of indices of basic events.
  ///
  /// @returns The total probability with the MCUB approximation.
  double ProbMcub(const std::vector<CutSet>& min_cut_sets) noexcept;

  /// Calculates probabilities
  /// using the Rare-Event approximation.
  ///
  /// @param[in] min_cut_sets Sets of indices of basic events.
  ///
  /// @returns The total probability with the rare-event approximation.
  double ProbRareEvent(const std::vector<CutSet>& min_cut_sets) noexcept;

  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set A cut set of indices of basic events.
  ///
  /// @returns The total probability of the set.
  ///
  /// @note O_avg(N) where N is the size of the passed set.
  double ProbAnd(const CutSet& cut_set) noexcept;

  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or cut sets.
  double CalculateTotalProbability() noexcept;

  /// Calculates exact probability
  /// of a function graph represented by its root BDD vertex.
  ///
  /// @param[in] vertex The root vertex of a function graph.
  /// @param[in] mark A flag to mark traversed vertices.
  ///
  /// @returns Probability value.
  ///
  /// @warning If a vertex is already marked with the input mark,
  ///          it will not be traversed and updated with a probability value.
  double CalculateProbability(const VertexPtr& vertex, bool mark) noexcept;

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

  GatePtr top_event_;  ///< Top gate of the passed fault tree.
  std::unique_ptr<BooleanGraph> bool_graph_;  ///< Indexation graph.
  std::unique_ptr<Bdd> bdd_graph_;  ///< The main BDD graph for analysis.
  std::string warnings_;  ///< Register warnings.

  /// Container for input basic events.
  std::unordered_map<std::string, BasicEventPtr> basic_events_;

  std::vector<BasicEventPtr> index_to_basic_;  ///< Indices to basic events.
  /// Indices of basic events.
  std::unordered_map<std::string, int> id_to_index_;
  std::vector<double> var_probs_;  ///< Variable probabilities.

  /// Minimal cut sets with indices of events.
  std::vector<CutSet> imcs_;
  /// Container for basic event indices that are in minimal cut sets.
  std::set<int> mcs_basic_events_;

  double p_total_;  ///< Total probability of the top event.
  bool current_mark_; ///< To keep track of BDD current mark.

  /// Container for basic event importance factors.
  std::unordered_map<std::string, ImportanceFactors> importance_;

  double p_time_;  ///< Time for probability calculations.
  double imp_time_;  ///< Time for importance calculations.
};

/// @class CutSetCalculator
/// Quantitative calculator of a probability value of a single cut set.
class CutSetCalculator {
 public:
  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set A cut set of signed indices of basic events.
  /// @param[in] var_probs Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability of the set.
  ///
  /// @pre Probability values are non-negative.
  /// @pre Absolute indices of events directly map to vector indices.
  ///
  /// @note O_avg(N) where N is the size of the passed set.
  template<typename CutSet>
  double Calculate(const CutSet& cut_set,
                   const std::vector<double>& var_probs) noexcept {
    if (cut_set.empty()) return 0;
    double p_sub_set = 1;  // 1 is for multiplication.
    for (int member : cut_set) {
      if (member > 0) {
        p_sub_set *= var_probs[member];
      } else {
        p_sub_set *= 1 - var_probs[std::abs(member)];
      }
    }
    return p_sub_set;
  }

  /// Checks the special case of a unity set with probability 1.
  ///
  /// @returns true if the Unity set is detected.
  ///
  /// @pre The unity set is indicated by a single empty set.
  template<typename CutSet>
  double CheckUnity(const std::vector<CutSet>& cut_sets) noexcept {
    return cut_sets.size() == 1 && cut_sets.front().empty();
  }
};

/// @class RareEventCalculator
/// Quantitative calculator of probability values
/// with the Rare-Event approximation.
class RareEventCalculator : private CutSetCalculator {
 public:
  /// Calculates probabilities
  /// using the Rare-Event approximation.
  ///
  /// @param[in] cut_sets A collection of sets of indices of basic events.
  /// @param[in] var_probs Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability with the rare-event approximation.
  ///
  /// @pre Absolute indices of events directly map to vector indices.
  ///
  /// @post The returned probability value may not be acceptable.
  ///       That is, it may be out of the acceptable [0, 1] range.
  ///       The caller of this function must decide
  ///       what to do in this case.
  template<typename CutSet>
  double Calculate(const std::vector<CutSet>& cut_sets,
                   const std::vector<double>& var_probs) noexcept {
    if (CutSetCalculator::CheckUnity(cut_sets)) return 1;
    double sum = 0;
    for (const auto& cut_set : cut_sets) {
      assert(!cut_set.empty() && "Detected an empty cut set.");
      sum += CutSetCalculator::Calculate(cut_set, var_probs);
    }
    return sum;
  }
};

//// @class McubCalculator
/// Quantitative calculator of probability values
/// with the Min-Cut-Upper Bound approximation.
class McubCalculator : private CutSetCalculator {
 public:
  /// Calculates probabilities
  /// using the minimal cut set upper bound (MCUB) approximation.
  ///
  /// @param[in] cut_sets A collection of sets of indices of basic events.
  /// @param[in] var_probs Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability with the MCUB approximation.
  template<typename CutSet>
  double Calculate(const std::vector<CutSet>& cut_sets,
                   const std::vector<double>& var_probs) noexcept {
    if (CutSetCalculator::CheckUnity(cut_sets)) return 1;
    double m = 1;
    for (const auto& cut_set : cut_sets) {
      assert(!cut_set.empty() && "Detected an empty cut set.");
      m *= 1 - CutSetCalculator::Calculate(cut_set, var_probs);
    }
    return 1 - m;
  }
};

/// @class ProbabilityAnalyzer
///
/// @tparam Algorithm Fault tree analysis algorithm.
/// @tparam Calculator Quantitative analysis calculator.
///
/// Fault tree analysis aware probability analyzer.
/// Probability analyzer provides the main engine for probability analysis.
template<typename Algorithm, typename Calculator>
class ProbabilityAnalyzer : public ProbabilityAnalysis {
 public:
  /// Constructs probability analyzer from a fault tree analyzer
  /// with the same algorithm.
  ///
  /// @param[in] fta Finished fault tree analyzer with results.
  explicit ProbabilityAnalyzer(const FaultTreeAnalyzer<Algorithm>* fta)
      : ProbabilityAnalysis::ProbabilityAnalysis(fta), fta_(fta) {
    var_probs_.push_back(-1);
    for (const BasicEventPtr& event : fta->graph()->basic_events()) {
      var_probs_.push_back(event->p());
    }
  }

  /// Runs the analysis to find the total probability.
  void Analyze() noexcept;

 private:
  const FaultTreeAnalyzer<Algorithm>* fta_;  ///< Finished fault tree analysis.
  std::vector<double> var_probs_;  ///< Variable probabilities.
};

template<typename Algorithm, typename Calculator>
void ProbabilityAnalyzer<Algorithm, Calculator>::Analyze() noexcept {
  CLOCK(p_time);
  LOG(DEBUG3) << "Calculating probabilities...";
  // Get the total probability.
  p_total_ = Calculator().Calculate(fta_->GetGeneratedMcs(), var_probs_);
  assert(p_total_ >= 0 && "The total probability is negative.");
  if (p_total_ > 1) {
    warnings_ += " Probability value exceeded 1 and was adjusted to 1.";
    p_total_ = 1;
  }
  LOG(DEBUG3) << "Finished probability calculations in " << DUR(p_time);
  p_time_ = DUR(p_time);
}

}  // namespace scram

#endif  // SCRAM_SRC_PROBABILITY_ANALYSIS_H_
