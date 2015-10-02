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
#include "logger.h"
#include "settings.h"

namespace scram {

/// @class ProbabilityAnalysis
/// Main quantitative analysis class.
class ProbabilityAnalysis : public Analysis {
 public:
  using GatePtr = std::shared_ptr<Gate>;

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

/// @class CutSetCalculator
/// Quantitative calculator of a probability value of a single cut set.
class CutSetCalculator {
 public:
  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @tparam CutSet  An iterable container of unique elements.
  ///
  /// @param[in] cut_set  A cut set of signed indices of basic events.
  /// @param[in] var_probs  Probabilities of events mapped to a vector.
  ///
  /// @returns The total probability of the set.
  ///
  /// @pre Probability values are non-negative.
  /// @pre Absolute indices of events directly map to vector indices.
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
  /// @tparam CutSet  An iterable container of unique elements.
  ///
  /// @param[in] cut_sets  Collection of ALL cut sets.
  ///
  /// @returns true if the Unity set is detected.
  ///
  /// @pre The unity set is indicated by a single empty set.
  /// @pre Provided cut sets are ALL the cut sets under consideration.
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
  /// @tparam CutSet  An iterable container of unique elements.
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
  /// @param[in] cut_sets  A collection of sets of indices of basic events.
  /// @param[in] var_probs  Probabilities of events mapped to a vector.
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

/// @class ProbabilityAnalyzerBase
/// Abstract base class for Probability analyzers.
class ProbabilityAnalyzerBase : public ProbabilityAnalysis {
 public:
  /// Constructs probability analyzer from a fault tree analyzer.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  template<typename Algorithm>
  explicit ProbabilityAnalyzerBase(const FaultTreeAnalyzer<Algorithm>* fta)
      : ProbabilityAnalysis::ProbabilityAnalysis(fta) {
    graph_ = fta->graph();
    var_probs_.push_back(-1);
    for (const BasicEventPtr& event : fta->graph()->basic_events()) {
      var_probs_.push_back(event->p());
    }
  }

  virtual ~ProbabilityAnalyzerBase() = 0;  ///< Abstract class.

  /// @returns The original Boolean graph from the fault tree analysis.
  const BooleanGraph* graph() const { return graph_; }

  /// @returns A modifiable mapping for probability values and indices.
  ///
  /// @pre Quantitative analyzers aware of how Probability analyzer works.
  /// @pre Quantitative analyzers will cleanup after themselves.
  ///
  /// @warning This is a temporary hack
  ///          due to tight coupling of Quantitative analyzers.
  std::vector<double>& var_probs() { return var_probs_; }

 protected:
  using BasicEventPtr = std::shared_ptr<BasicEvent>;

  const BooleanGraph* graph_;  ///< Boolean graph from the fault tree analysis.
  std::vector<double> var_probs_;  ///< Variable probabilities.
};

/// @class ProbabilityAnalyzer
/// Fault-tree-analysis-aware probability analyzer.
/// Probability analyzer provides the main engine for probability analysis.
///
/// @tparam Algorithm  Fault tree analysis algorithm.
/// @tparam Calculator  Quantitative analysis calculator.
template<typename Algorithm, typename Calculator>
class ProbabilityAnalyzer : public ProbabilityAnalyzerBase {
 public:
  /// Constructs probability analyzer from a fault tree analyzer
  /// with the same algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  explicit ProbabilityAnalyzer(const FaultTreeAnalyzer<Algorithm>* fta)
      : ProbabilityAnalyzerBase::ProbabilityAnalyzerBase(fta),
        fta_(fta) {
    calc_ = std::unique_ptr<Calculator>(new Calculator());
  }

  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or cut sets.
  double CalculateTotalProbability() noexcept {
    return calc_->Calculate(fta_->algorithm()->cut_sets(), var_probs_);
  }

  /// @returns Fault tree analyzer as given to this analyzer.
  ///
  /// @todo Remove after decoupling analyses.
  const FaultTreeAnalyzer<Algorithm>* fta() { return fta_; }

 private:
  const FaultTreeAnalyzer<Algorithm>* fta_;  ///< Finished fault tree analysis.
  std::unique_ptr<Calculator> calc_;  ///< Provider of the calculation logic.
};

/// @class ProbabilityAnalyzer<Algorithm, Bdd>
/// Specialization of probability analyzer with Binary Decision Diagrams.
/// The quantitative analysis is done with BDD.
///
/// @tparam Algorithm  Fault tree analysis algorithm.
template<typename Algorithm>
class ProbabilityAnalyzer<Algorithm, Bdd> : public ProbabilityAnalyzerBase {
 public:
  /// Constructs probability analyzer from a fault tree analyzer
  /// with the same algorithm.
  ///
  /// @param[in] fta  Finished fault tree analyzer with results.
  explicit ProbabilityAnalyzer(const FaultTreeAnalyzer<Algorithm>* fta);

  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or cut sets.
  double CalculateTotalProbability() noexcept;

  /// @returns Fault tree analyzer as given to this analyzer.
  ///
  /// @todo Remove after decoupling analyses.
  const FaultTreeAnalyzer<Algorithm>* fta() { return fta_; }

  /// @returns Binary decision diagram used for calculations.
  Bdd* bdd_graph() { return bdd_graph_.get(); }

 private:
  using VertexPtr = std::shared_ptr<Vertex>;
  using ItePtr = std::shared_ptr<Ite>;

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

  const FaultTreeAnalyzer<Algorithm>* fta_;  ///< Finished fault tree analysis.
  std::unique_ptr<BooleanGraph> bool_graph_;  ///< Indexation graph.
  std::unique_ptr<Bdd> bdd_graph_;  ///< The main BDD graph for analysis.
  bool current_mark_; ///< To keep track of BDD current mark.
};

template<typename Algorithm>
ProbabilityAnalyzer<Algorithm, Bdd>::ProbabilityAnalyzer(
    const FaultTreeAnalyzer<Algorithm>* fta)
    : ProbabilityAnalyzerBase::ProbabilityAnalyzerBase(fta),
      fta_(fta),
      current_mark_(false) {
  CLOCK(main_time);
  CLOCK(ft_creation);
  bool_graph_ = std::unique_ptr<BooleanGraph>(
      new BooleanGraph(fta_->top_event(), kSettings_.ccf_analysis()));
  LOG(DEBUG2) << "Boolean graph is created in " << DUR(ft_creation);
  assert(fta_->graph()->basic_events() == bool_graph_->basic_events() &&
         "Boolean graph is not stable, or the fault tree is corrupted!");
  CLOCK(prep_time);  // Overall preprocessing time.
  LOG(DEBUG2) << "Preprocessing...";
  Preprocessor* preprocessor = new CustomPreprocessor<Bdd>(bool_graph_.get());
  preprocessor->Run();
  delete preprocessor;  // No exceptions are expected.
  LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);

  CLOCK(bdd_time);  // BDD based calculation time.
  LOG(DEBUG2) << "Creating BDD for ProbabilityAnalysis...";
  bdd_graph_ = std::unique_ptr<Bdd>(new Bdd(bool_graph_.get(), kSettings_));
  LOG(DEBUG2) << "BDD is created in " << DUR(bdd_time);
  analysis_time_ = DUR(main_time);
}

template<typename Algorithm>
double
ProbabilityAnalyzer<Algorithm, Bdd>::CalculateTotalProbability() noexcept {
  CLOCK(calc_time);  // BDD based calculation time.
  LOG(DEBUG4) << "Calculating probability with BDD...";
  current_mark_ = !current_mark_;
  double prob = ProbabilityAnalyzer::CalculateProbability(
      bdd_graph_->root().vertex,
      current_mark_);
  if (bdd_graph_->root().complement) prob = 1 - prob;
  LOG(DEBUG4) << "Calculated probability " << prob << " in " << DUR(calc_time);
  return prob;
}

template<typename Algorithm>
double ProbabilityAnalyzer<Algorithm, Bdd>::CalculateProbability(
    const VertexPtr& vertex,
    bool mark) noexcept {
  if (vertex->terminal()) return 1;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return ite->prob();
  ite->mark(mark);
  double var_prob = 0;
  if (ite->module()) {
    const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
    var_prob = ProbabilityAnalyzer::CalculateProbability(res.vertex, mark);
    if (res.complement) var_prob = 1 - var_prob;
  } else {
    var_prob = var_probs_[ite->index()];
  }
  double high = ProbabilityAnalyzer::CalculateProbability(ite->high(), mark);
  double low = ProbabilityAnalyzer::CalculateProbability(ite->low(), mark);
  if (ite->complement_edge()) low = 1 - low;
  ite->prob(var_prob * high + (1 - var_prob) * low);
  return ite->prob();
}

}  // namespace scram

#endif  // SCRAM_SRC_PROBABILITY_ANALYSIS_H_
