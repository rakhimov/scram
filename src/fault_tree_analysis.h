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

namespace scram {

/// @class FaultTreeAnalysis
/// Fault tree analysis functionality.
class FaultTreeAnalysis {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;

  /// Traverses a valid fault tree from the root gate to collect
  /// databases of events, gates, and other members of the fault tree.
  /// The passed fault tree must be pre-validated without cycles, and
  /// its events must be fully initialized. It is assumed that analysis
  /// is done only once.
  /// @param[in] root The top event of the fault tree to analyze.
  /// @param[in] limit_order The maximum limit on minimal cut sets' order.
  /// @param[in] ccf_analysis Whether or not expand CCF group basic events.
  /// @throws InvalidArgument if any of the parameters are invalid.
  explicit FaultTreeAnalysis(const GatePtr& root, int limit_order = 20,
                             bool ccf_analysis = false);

  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initializing the tree with or
  /// without its probabilities. Underlying objects may throw errors
  /// if the fault tree has initialization issues. However, there is no
  /// guarantee for that. This function is expected to be called only once.
  void Analyze();

  /// @returns The top gate.
  inline const GatePtr& top_event() const { return top_event_; }

  /// @returns The container of intermediate events.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, GatePtr>&
      inter_events() const {
    return inter_events_;
  }

  /// @returns The container of all basic events of this tree. If CCF analysis
  ///          is requested, this container includes the basic events that
  ///          represent common cause failure.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      basic_events() const {
    return basic_events_;
  }

  /// @returns Basic events that are in some CCF groups.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      ccf_events() const {
    return ccf_events_;
  }

  /// @returns The container of house events of this tree.
  inline const boost::unordered_map<std::string, HouseEventPtr>&
      house_events() const {
    return house_events_;
  }

  /// @returns Set with minimal cut sets.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::set< std::set<std::string> >& min_cut_sets() const {
    return min_cut_sets_;
  }

  /// @returns Collection of basic events that are in the minimal cut sets.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      mcs_basic_events() const {
    return mcs_basic_events_;
  }

  /// @returns The maximum order of the found minimal cut sets.
  inline int max_order() const { return max_order_; }

  /// @returns Warnings generated upon analysis.
  inline const std::string& warnings() const { return warnings_; }

  /// @returns Analysis time spent on finding minimal cut sets.
  inline double analysis_time() const { return analysis_time_; }

 private:
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Gathers information about the correctly initialized fault tree. Databases
  /// for events are manipulated to best reflect the state and structure
  /// of the fault tree. This function must be called after validation.
  /// This function must be called before any analysis is performed because
  /// there would not be necessary information available for analysis like
  /// primary events of this fault tree. Moreover, all the nodes of this
  /// fault tree are expected to be defined fully and correctly.
  /// Gates are marked upon visit. The mark is checked to prevent revisiting.
  /// @param[in] gate The gate to start traversal from.
  void GatherEvents(const GatePtr& gate);

  /// Traverses formulas recursively to find all events.
  /// @param[in] formula The formula to get events from.
  void GatherEvents(const FormulaPtr& formula);

  /// Cleans marks from nodes that were traversed.
  /// Marks are set to empty strings. This is important because other code
  /// may assume that marks are empty.
  void CleanMarks();

  /// Picks basic events created by CCF groups.
  /// param[out] basic_events Container for newly created basic events.
  void GatherCcfBasicEvents(
      boost::unordered_map<std::string, BasicEventPtr>* basic_events);

  /// Converts minimal cut sets from indices to strings for future reporting.
  /// This function also detects basic events in minimal cut sets.
  /// @param[in] imcs Min cut sets with indices of events.
  void SetsToString(const std::vector< std::set<int> >& imcs);

  std::vector<BasicEventPtr> int_to_basic_;  ///< Indices to basic events.
  boost::unordered_map<std::string, int> all_to_int_;  ///< All event indices.

  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;
  bool ccf_analysis_;  ///< A flag to include CCF groups in fault trees.

  GatePtr top_event_;  ///< Top event of this fault tree.

  /// Container for intermediate events.
  boost::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for basic events.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for house events of the tree.
  boost::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for basic events that are identified to be in some CCF group.
  /// These basic events are not necessarily in the same CCF group.
  boost::unordered_map<std::string, BasicEventPtr> ccf_events_;

  /// Container for minimal cut sets.
  std::set< std::set<std::string> > min_cut_sets_;

  /// Container for basic events in minimal cut sets.
  boost::unordered_map<std::string, BasicEventPtr> mcs_basic_events_;

  int max_order_;  ///< Maximum order of minimal cut sets.
  std::string warnings_;  ///< Generated warnings in analysis.
  double analysis_time_;  ///< Time taken by the core analysis.
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
