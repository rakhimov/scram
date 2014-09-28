/// @file fault_tree_analysis.h
/// Fault Tree Analysis.
#ifndef SCRAM_FAULT_TREE_ANALYSIS_H_
#define SCRAM_FAULT_TREE_ANALYSIS_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "error.h"
#include "event.h"
#include "fault_tree.h"
#include "superset.h"

class FaultTreeAnalysisTest;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

typedef boost::shared_ptr<scram::Superset> SupersetPtr;

typedef boost::shared_ptr<scram::FaultTree> FaultTreePtr;

namespace scram {

class Reporter;

/// @class FaultTreeAnalysis
/// Fault tree analysis functionality.
class FaultTreeAnalysis {
  friend class ::FaultTreeAnalysisTest;
  friend class Reporter;

 public:
  /// The main constructor of the Fault Tree Analysis.
  /// @param[in] analysis The type of fault tree analysis.
  /// @param[in] approx The kind of approximation for probability calculations.
  /// @param[in] limit_order The maximum limit on minimal cut sets' order.
  /// @param[in] nsums The number of sums in the probability series.
  /// @param[in] cut_off The cut-off probability for cut sets.
  /// @throws ValueError if any of the parameters are invalid.
  explicit FaultTreeAnalysis(std::string analysis, std::string approx = "no",
                             int limit_order = 20, int nsums = 1000000,
                             double cut_off = 1e-8);

  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initilizing the tree with or
  /// without its probabilities. Underlying objects may throw errors
  /// if the fault tree has initialization issues. However, there is no
  /// quarantee for that.
  /// @param[in] fault_tree Valid Fault Tree.
  /// @param[in] prob_requested Indication for the probability calculations.
  /// @note Cut set generator: O_avg(N) O_max(N)
  void Analyze(const FaultTreePtr& fault_tree, bool prob_requested);

  /// @returns The total probability calculated by the analysis.
  /// @note The user should make sure that the analysis is actually done.
  inline double p_total() { return p_total_; }

  /// @returns Set with minimal cut sets.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::set< std::set<std::string> >& min_cut_sets() {
    return min_cut_sets_;
  }

  /// @returns Map with minimal cut sets and their probabilities.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::set<std::string>, double >& prob_of_min_sets() {
    return prob_of_min_sets_;
  }

  /// @returns Map with primary events and their contribution.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::string, double >& imp_of_primaries() {
    return imp_of_primaries_;
  }

 private:
  /// Preprocesses the fault tree.
  /// Merges similar gates.
  /// @param[in] gate The starting gate to traverse the tree. This is for
  ///                 recursive purposes.
  void PreprocessTree(const GatePtr& gate);

  /// Traverses the fault tree and expands it into sets of gates and events.
  /// @param[in] set_with_gates A superset with gates.
  /// @param[in] cut_sets Container for cut sets upon tree expansion.
  void ExpandTree(const SupersetPtr& set_with_gates,
                  std::vector<SupersetPtr>* cut_sets);

  /// Expands the children of a top or intermediate event to Supersets.
  /// @param[in] inter_index The index number of the parent node.
  /// @param[out] sets The final Supersets from the children.
  /// @throws ValueError if the parent's gate is not recognized.
  /// @note The final sets are dependent on the gate of the parent.
  /// @note O_avg(N, N*logN) O_max(N^2, N^3*logN) where N is a children number.
  void ExpandSets(int inter_index, std::vector<SupersetPtr>* sets);

  /// Expands sets for OR operator.
  /// @param[in] mult The positive(1) or negative(-1) event indicator.
  /// @param[in] events_children The indices of the children of the event.
  /// @param[out] sets The final Supersets generated for OR operator.
  /// @note O_avg(N) O_max(N^2)
  void SetOr(int mult, const std::vector<int>& events_children,
             std::vector<SupersetPtr>* sets);

  /// Expands sets for AND operator.
  /// @param[in] mult The positive(1) or negative(-1) event indicator.
  /// @param[in] events_children The indices of the children of the event.
  /// @param[out] sets The final Supersets generated for OR operator.
  /// @note O_avg(N*logN) O_max(N*logN) where N is the number of children.
  void SetAnd(int mult, const std::vector<int>& events_children,
              std::vector<SupersetPtr>* sets);

  /// Finds minimal cut sets from cut sets.
  /// Applys rule 4 to reduce unique cut sets to minimal cut sets.
  /// @param[in] cut_sets Cut sets with primary events.
  /// @param[in] mcs_lower_order Reference minimal cut sets of some order.
  /// @param[in] min_order The order of sets to become minimal.
  /// @note T_avg(N^3 + N^2*logN + N*logN) = O_avg(N^3)
  void FindMcs(const std::vector< const std::set<int>* >& cut_sets,
               const std::vector< std::set<int> >& mcs_lower_order,
               int min_order);

  // -------------------- Algorithm for Cut Sets and Probabilities -----------
  /// Assigns an index to each primary event, and then populates with this
  /// indices new databases of minimal cut sets and primary to integer
  /// converting maps.
  /// In addition, this function copies all events from
  /// the fault tree for future reference.
  /// @param[in] fault_tree Fault Tree with events and gates.
  /// @note O_avg(N) O_max(N^2) where N is the total number of tree nodes.
  void AssignIndices(const FaultTreePtr& fault_tree);

  /// Converts minimal cut sets from indices to strings for future reporting.
  void SetsToString();

  /// Calculates a probability of a set of minimal cut sets, which are in OR
  /// relationship with each other. This function is a brute force probability
  /// calculation without approximations.
  /// @param[in] nsums The number of sums in the series.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  /// @returns The total probability.
  /// @note This function drastically modifies min_cut_sets by deleting
  /// sets inside it. This is for better performance.
  ///
  /// @note O_avg(M*logM*N*2^N) where N is the number of sets, and M is
  /// the average size of the sets.
  double ProbOr(int nsums, std::set< std::set<int> >* min_cut_sets);

  /// Calculates a probability of a minimal cut set, whose members are in AND
  /// relationship with each other. This function assumes independence of each
  /// member.
  /// @param[in] min_cut_set A set of indices of primary events.
  /// @returns The total probability.
  /// @note O_avg(N) where N is the size of the passed set.
  double ProbAnd(const std::set<int>& min_cut_set);

  /// Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  /// @param[in] el A set of indices of primary events.
  /// @param[in] set Sets of indices of primary events.
  /// @param[out] combo_set A final set resulting from joining el and sets.
  /// @note O_avg(N*M*logM) where N is the size of the set, and M is the
  /// average size of the elements.
  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >* combo_set);

  std::vector< std::set<int> > imcs_;  ///< Min cut sets with indices of events.
  /// Indices min cut sets to strings min cut sets mapping.
  std::map< std::set<int>, std::set<std::string> > imcs_to_smcs_;

  std::vector<PrimaryEventPtr> int_to_primary_;  ///< Indices to primary events.
  /// Indices of primary events.
  boost::unordered_map<std::string, int> primary_to_int_;
  std::vector<double> iprobs_;  ///< Holds probabilities of primary events.

  int top_event_index_;  ///< The index of the top event.
  /// Intermediate events from indices.
  boost::unordered_map<int, GatePtr> int_to_inter_;
  /// Indices of intermediate events.
  boost::unordered_map<std::string, int> inter_to_int_;
  // -----------------------------------------------------------------
  // ---- Algorithm for Equation Construction for Monte Carlo Sim -------
  /// Generates positive and negative terms of probability equation expansion.
  /// @param[in] sign The sign of the series. Odd int is '+', even int is '-'.
  /// @param[in] nsums The number of sums in the series.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  void MProbOr(int sign, int nsums, std::set< std::set<int> >* min_cut_sets);

  /// Performs Monte Carlo Simulation.
  /// @todo Implement the simulation.
  void MSample();

  std::vector< std::set<int> > pos_terms_;  ///< Plus terms of the equation.
  std::vector< std::set<int> > neg_terms_;  ///< Minus terms of the equation.
  std::vector<double> sampled_results_;  ///< Storage for sampled values.
  // -----------------------------------------------------------------
  // ----------------------- Member Variables of this Class -----------------
  /// This member is used to provide any warnings about assumptions,
  /// calculations, and settings. These warnings must be written into output
  /// file.
  std::string warnings_;

  /// Type of analysis to be performed.
  std::string analysis_;

  /// Approximations for probability calculations.
  std::string approx_;

  /// Indicator if probability calculations are requested.
  bool prob_requested_;

  /// Number of sums in series expansion for probability calculations.
  int nsums_;

  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;

  /// Cut-off probability for minimal cut sets.
  double cut_off_;

  /// Top event.
  GatePtr top_event_;

  /// Holder for intermediate events.
  boost::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Container for minimal cut sets.
  std::set< std::set<std::string> > min_cut_sets_;

  /// Container for minimal cut sets and their respective probabilities.
  std::map< std::set<std::string>, double > prob_of_min_sets_;

  /// Container for minimal cut sets ordered by their probabilities.
  std::multimap< double, std::set<std::string> > ordered_min_sets_;

  /// Container for primary events and their contribution.
  std::map< std::string, double > imp_of_primaries_;

  /// Container for primary events ordered by their contribution.
  std::multimap< double, std::string > ordered_primaries_;

  /// Maximum order of minimal cut sets.
  int max_order_;

  /// The number of minimal cut sets with higher than cut-off probability.
  int num_prob_mcs_;

  /// Total probability of the top event.
  double p_total_;

  // Time logging
  double exp_time_;  ///< Expansion of tree gates time.
  double mcs_time_;  ///< Time for MCS generation.
  double p_time_;  ///< Time for probability calculations.

  /// Track if the gates are repeated upon expansion.
  boost::unordered_map<int, std::vector<SupersetPtr> > repeat_exp_;
};

}  // namespace scram

#endif  // SCRAM_FAULT_TREE_ANALYSIS_H_
