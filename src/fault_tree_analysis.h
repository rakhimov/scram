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

class FaultTreeAnalysisTest;
class PerformanceTest;

namespace scram {

class Reporter;

/// @class FaultTreeAnalysis
/// Fault tree analysis functionality.
class FaultTreeAnalysis {
  friend class ::FaultTreeAnalysisTest;
  friend class ::PerformanceTest;
  friend class Reporter;

 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;

  /// Traverses a valid fault tree from the root gate to collect
  /// databases of events, gates, and other members of the fault tree.
  /// The passed fault tree must be pre-validated without cycles, and
  /// its events must be fully initialized.
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
  /// guarantee for that.
  void Analyze();

  /// @returns The top gate.
  inline GatePtr& top_event() { return top_event_; }

  /// @returns The container of intermediate events.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, GatePtr>& inter_events() {
    return inter_events_;
  }

  /// @returns The container of all basic events of this tree. If CCF analysis
  ///          is requested, this container includes the basic events that
  ///          represent common cause failure.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      basic_events() {
    return basic_events_;
  }

  /// @returns Basic events that are in some CCF groups.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      ccf_events() {
    return ccf_events_;
  }

  /// @returns The container of house events of this tree.
  inline const boost::unordered_map<std::string, HouseEventPtr>&
      house_events() {
    return house_events_;
  }

  /// @returns The original number of basic events without new CCF basic events.
  inline int num_basic_events() { return num_basic_events_; }

  /// @returns Set with minimal cut sets.
  /// @note The user should make sure that the analysis is actually done.
  inline const std::set< std::set<std::string> >& min_cut_sets() const {
    return min_cut_sets_;
  }

  /// @returns The maximum order of the found minimal cut sets.
  inline int max_order() const { return max_order_; }

  /// @returns Warnings generated upon analysis.
  inline const std::string& warnings() const { return warnings_; }

 private:
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<Event> EventPtr;

  /// Gathers information about the correctly initialized fault tree. Databases
  /// for events are manipulated to best reflect the state and structure
  /// of the fault tree. This function must be called after validation.
  /// This function must be called before any analysis is performed because
  /// there would not be necessary information available for analysis like
  /// primary events of this fault tree. Moreover, all the nodes of this
  /// fault tree are expected to be defined fully and correctly.
  /// @throws LogicError if the fault tree is not fully defined or some
  ///                    information is missing.
  void SetupForAnalysis();

  /// Traverses gates recursively to find all intermediate events.
  /// Gates are marked upon visit.
  /// @param[in] gate The gate to start traversal from.
  void GatherInterEvents(const GatePtr& gate);

  /// Picks primary events of this tree.
  /// Populates the container of primary events.
  void GatherPrimaryEvents();

  /// Picks primary events of the specified gate.
  /// The primary events are put into the appropriate container.
  /// @param[in] gate The gate to get primary events from.
  void GetPrimaryEvents(const GatePtr& gate);

  /// Picks basic events created by CCF groups.
  /// Populates the container of basic and primary events.
  void GatherCcfBasicEvents();

  /// Converts minimal cut sets from indices to strings for future reporting.
  /// This function also removes house events from minimal cut sets.
  /// @param[in] imcs Min cut sets with indices of events.
  void SetsToString(const std::vector< std::set<int> >& imcs);

  std::vector<PrimaryEventPtr> int_to_basic_;  ///< Indices to basic events.

  int top_event_index_;  ///< The index of the top event.
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

  int max_order_;  ///< Maximum order of minimal cut sets.
  double analysis_time_;  ///< Time taken by the core analysis.

  /// The number of unique events in the minimal cut sets.
  /// CCF events are treated as separate events from their group members.
  int num_mcs_events_;
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_ANALYSIS_H_
