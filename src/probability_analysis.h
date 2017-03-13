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

#include <utility>
#include <vector>

#include "analysis.h"
#include "bdd.h"
#include "fault_tree_analysis.h"
#include "pdag.h"

namespace scram {

namespace mef {
class MissionTime;
}

namespace core {

/// Safety Integrity Level metrics.
///
/// @note Averages and histograms are with respect to time.
struct Sil {
  double pfd_avg = 0;  ///< The average probability of failure on demand (PFD).
  double pfh_avg = 0;  ///< The average probability of failure hourly (PFH).

  /// The SIL PFD and PFD fractions histogram in reverse order, i.e., 4 to 1.
  /// The starting boundary is implicitly 0.
  /// The last boundary is explicit 1.
  /// The range is half open: (lower-bound, upper-bound].
  /// @{
  std::array<std::pair<const double, double>, 6> pfd_fractions{
      {{1e-5, 0}, {1e-4, 0}, {1e-3, 0}, {1e-2, 0}, {1e-1, 0}, {1, 0}}};
  std::array<std::pair<const double, double>, 6> pfh_fractions{
      {{1e-9, 0}, {1e-8, 0}, {1e-7, 0}, {1e-6, 0}, {1e-5, 0}, {1, 0}}};
  /// @}
};

/// Main quantitative analysis class.
class ProbabilityAnalysis : public Analysis {
 public:
  /// Probability analysis
  /// with the results of qualitative analysis.
  ///
  /// @param[in] fta  Fault tree analysis with results.
  /// @param[in] mission_time  The mission time expression of the model.
  ///
  /// @pre The underlying fault tree must not have changed in any way
  ///      since the fault tree analysis finished.
  ProbabilityAnalysis(const FaultTreeAnalysis* fta,
                      mef::MissionTime* mission_time);

  virtual ~ProbabilityAnalysis() = default;

  /// Performs quantitative analysis on the supplied fault tree.
  ///
  /// @pre Analysis is called only once.
  ///
  /// @post The mission time expression has its original value.
  void Analyze() noexcept;

  /// @returns The total probability calculated by the analysis.
  ///
  /// @pre The analysis is done.
  double p_total() const { return p_total_; }

  /// @returns The probability values over the mission time in time steps.
  ///          The empty container implies no calculation has been done.
  ///
  /// @pre The analysis is done.
  const std::vector<std::pair<double, double>>& p_time() const {
    return p_time_;
  }

  /// @returns The Safety Integrity Level calculation results.
  ///
  /// @pre The analysis is done with a request for the SIL.
  const Sil& sil() const {
    assert(sil_ && "The SIL is not done!");
    return *sil_;
  }

 protected:
  /// @returns The mission time expression of the model.
  mef::MissionTime& mission_time() { return *mission_time_; }

 private:
  /// Calculates the total probability.
  ///
  /// @returns The total probability of the graph or products.
  virtual double CalculateTotalProbability() noexcept = 0;

  /// Calculates the probability evolution through the mission time.
  ///
  /// @returns The probabilities at time steps.
  virtual std::vector<std::pair<double, double>>
  CalculateProbabilityOverTime() noexcept = 0;

  /// Computes probability metrics related to the SIL.
  void ComputeSil() noexcept;

  double p_total_;  ///< Total probability of the top event.
  mef::MissionTime* mission_time_;  ///< The mission time expression.
  std::vector<std::pair<double, double>> p_time_;  ///< {probability, time}.
  std::unique_ptr<Sil> sil_;  ///< The Safety Integrity Level results.
};

/// Quantitative calculator of a probability value of a single cut set.
class CutSetProbabilityCalculator {
 public:
  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set  A cut set with positive indices of basic events.
  /// @param[in] p_vars  Probabilities of events mapped by the variable indices.
  ///
  /// @returns The total probability of the cut set.
  /// @returns 1 for an empty cut set indicating the base set.
  ///
  /// @pre The cut set doesn't contain complements.
  /// @pre Probability values are non-negative.
  /// @pre Indices of events directly map to vector indices.
  double Calculate(const std::vector<int>& cut_set,
                   const Pdag::IndexMap<double>& p_vars) noexcept;
};

class Zbdd;  // The container of analysis products for computations.

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
  double Calculate(const Zbdd& cut_sets,
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
  double Calculate(const Zbdd& cut_sets,
                   const Pdag::IndexMap<double>& p_vars) noexcept;
};

/// Base class for Probability analyzers.
class ProbabilityAnalyzerBase : public ProbabilityAnalysis {
 public:
  /// Constructs probability analyzer from a fault tree analyzer.
  ///
  /// @tparam Algorithm  Qualitative analysis algorithm.
  ///
  /// @copydetails ProbabilityAnalysis::ProbabilityAnalysis
  template <class Algorithm>
  ProbabilityAnalyzerBase(const FaultTreeAnalyzer<Algorithm>* fta,
                          mef::MissionTime* mission_time)
      : ProbabilityAnalysis(fta, mission_time),
        graph_(fta->graph()),
        products_(fta->algorithm()->products()) {
    ExtractVariableProbabilities();
  }

  /// @returns The original PDAG from the fault tree analyzer.
  const Pdag* graph() const { return graph_; }

  /// @returns The resulting products of the fault tree analyzer.
  const Zbdd& products() const { return products_; }

  /// @returns A mapping for probability values with indices.
  const Pdag::IndexMap<double>& p_vars() const { return p_vars_; }

 protected:
  ~ProbabilityAnalyzerBase() override = default;

 private:
  /// Calculates the total probability
  /// with a different set of probability values
  /// than the one given upon construction.
  ///
  /// @param[in] p_vars  A map of probabilities of the graph variables.
  ///                    The indices of the variables must map
  ///                    exactly to the values.
  ///
  /// @returns The total probability calculated with the given values.
  virtual double CalculateTotalProbability(
      const Pdag::IndexMap<double>& p_vars) noexcept = 0;

  double CalculateTotalProbability() noexcept final {
    return this->CalculateTotalProbability(p_vars_);
  }

  std::vector<std::pair<double, double>>
  CalculateProbabilityOverTime() noexcept final;

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
  const Zbdd& products_;  ///< A collection of products.
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

  double CalculateTotalProbability(
      const Pdag::IndexMap<double>& p_vars) noexcept final {
    return calc_.Calculate(ProbabilityAnalyzerBase::products(), p_vars);
  }

 private:
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
  /// @copydetails ProbabilityAnalysis::ProbabilityAnalysis
  template <class Algorithm>
  ProbabilityAnalyzer(const FaultTreeAnalyzer<Algorithm>* fta,
                      mef::MissionTime* mission_time)
      : ProbabilityAnalyzerBase(fta, mission_time),
        current_mark_(false),
        owner_(true) {
    CreateBdd(*fta);
  }

  /// Reuses BDD structures from Fault tree analyzer.
  ///
  /// @copydetails ProbabilityAnalysis::ProbabilityAnalysis
  ///
  /// @pre BDD is fully formed and used.
  ///
  /// @post FaultTreeAnalyzer is not corrupted
  ///       by use of its BDD internals.
  ProbabilityAnalyzer(FaultTreeAnalyzer<Bdd>* fta,
                      mef::MissionTime* mission_time);

  /// Deletes the PDAG and BDD
  /// only if ProbabilityAnalyzer is the owner of them.
  ~ProbabilityAnalyzer() noexcept;

  /// @returns Binary decision diagram used for calculations.
  Bdd* bdd_graph() { return bdd_graph_; }

  double CalculateTotalProbability(
      const Pdag::IndexMap<double>& p_vars) noexcept final;

 private:
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
