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

class FaultTreeAnalysisTest;
class PerformanceTest;

namespace scram {

class PrimaryEvent;
class BasicEvent;
class FaultTree;
class Reporter;

/// @class FaultTreeAnalysis
/// Fault tree analysis functionality.
class FaultTreeAnalysis {
  friend class ::FaultTreeAnalysisTest;
  friend class ::PerformanceTest;
  friend class Reporter;

 public:
  typedef boost::shared_ptr<FaultTree> FaultTreePtr;

  /// The main constructor of Fault Tree Analysis.
  /// @param[in] limit_order The maximum limit on minimal cut sets' order.
  /// @param[in] ccf_analysis Whether or not expand CCF group basic events.
  /// @throws InvalidArgument if any of the parameters are invalid.
  explicit FaultTreeAnalysis(int limit_order = 20, bool ccf_analysis = false);

  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initializing the tree with or
  /// without its probabilities. Underlying objects may throw errors
  /// if the fault tree has initialization issues. However, there is no
  /// guarantee for that.
  /// @param[in] fault_tree Valid Fault Tree.
  /// @note Cut set generator: O_avg(N) O_max(N)
  void Analyze(const FaultTreePtr& fault_tree);

  /// @returns Set with minimal cut sets.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::set< std::set<std::string> >& min_cut_sets() const {
    return min_cut_sets_;
  }

  /// @returns The maximum order of the found minimal cut sets.
  inline int max_order() const { return max_order_; }

  /// @returns Warnings generated upon analysis.
  inline const std::string warnings() const { return warnings_; }

 private:
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

  /// Converts minimal cut sets from indices to strings for future reporting.
  /// This function also removes house events from minimal cut sets.
  /// @param[in] imcs Min cut sets with indices of events.
  void SetsToString(const std::vector< std::set<int> >& imcs);

  std::vector<PrimaryEventPtr> int_to_basic_;  ///< Indices to basic events.

  int top_event_index_;  ///< The index of the top event.
  std::string top_event_name_;  ///< The original name of the top event.
  int num_gates_;  ///< The number of the gates.
  int num_basic_events_;  ///< The number of the basic events.

  /// Indices of all events.
  boost::unordered_map<std::string, int> all_to_int_;

  /// This member is used to provide any warnings about assumptions,
  /// calculations, and settings. These warnings must be written into output
  /// file.
  std::string warnings_;

  /// A flag to include CCF groups in fault trees.
  bool ccf_analysis_;

  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;

  /// Container for basic events.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for minimal cut sets.
  std::set< std::set<std::string> > min_cut_sets_;

  int max_order_;  ///< Maximum order of minimal cut sets.
  double analysis_time_;  ///< Time taken by the core analysis.

  /// The number of unique events in the minimal cut sets.
  /// CCF events are treated as separate events from their group members.
  int num_mcs_events_;
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
