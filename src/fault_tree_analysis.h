/// @file fault_tree_analysis.h
/// Fault Tree Analysis.
#ifndef SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
#define SCRAM_SRC_FAULT_TREE_ANALYSIS_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "event.h"
#include "fault_tree.h"

#include "indexed_fault_tree.h"

class FaultTreeAnalysisTest;
class PerformanceTest;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;
typedef boost::shared_ptr<scram::BasicEvent> BasicEventPtr;

typedef boost::shared_ptr<scram::FaultTree> FaultTreePtr;

namespace scram {

class Reporter;

/// @class FaultTreeAnalysis
/// Fault tree analysis functionality.
class FaultTreeAnalysis {
  friend class ::FaultTreeAnalysisTest;
  friend class ::PerformanceTest;
  friend class Reporter;

 public:
  /// The main constructor of Fault Tree Analysis.
  /// @param[in] limit_order The maximum limit on minimal cut sets' order.
  /// @throws InvalidArgument if any of the parameters are invalid.
  explicit FaultTreeAnalysis(int limit_order = 20);

  ~FaultTreeAnalysis() { delete indexed_tree_; }

  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initilizing the tree with or
  /// without its probabilities. Underlying objects may throw errors
  /// if the fault tree has initialization issues. However, there is no
  /// quarantee for that.
  /// @param[in] fault_tree Valid Fault Tree.
  /// @note Cut set generator: O_avg(N) O_max(N)
  void Analyze(const FaultTreePtr& fault_tree);

  /// @returns Set with minimal cut sets.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::set< std::set<std::string> >& min_cut_sets() {
    return min_cut_sets_;
  }

 private:
  /// Assigns an index to each primary event, and then populates with this
  /// indices new databases of minimal cut sets and primary to integer
  /// converting maps.
  /// In addition, this function copies all events from
  /// the fault tree for future reference.
  /// @param[in] fault_tree Fault Tree with events and gates.
  /// @note O_avg(N) O_max(N^2) where N is the total number of tree nodes.
  void AssignIndices(const FaultTreePtr& fault_tree);

  /// Converts minimal cut sets from indices to strings for future reporting.
  /// This function also removes house events from minimal cut sets.
  /// @param[in] imcs Min cut sets with indices of events.
  /// @todo House events should be removed in pre-processing state.
  void SetsToString(const std::vector< std::set<int> >& imcs);

  std::vector<PrimaryEventPtr> int_to_primary_;  ///< Indices to primary events.

  int top_event_index_;  ///< The index of the top event.
  std::string top_event_name_;  ///< The original name of the top event.
  int num_gates_;  ///< The number of the gates.

  /// Indices of all events.
  boost::unordered_map<std::string, int> all_to_int_;

  /// This member is used to provide any warnings about assumptions,
  /// calculations, and settings. These warnings must be written into output
  /// file.
  std::string warnings_;

  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;

  /// Container for basic events.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for minimal cut sets.
  std::set< std::set<std::string> > min_cut_sets_;

  /// Maximum order of minimal cut sets.
  int max_order_;

  // Time logging
  double exp_time_;  ///< Expansion of tree gates time.
  double mcs_time_;  ///< Time for MCS generation.

  /// Indexed fault tree.
  IndexedFaultTree* indexed_tree_;
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
