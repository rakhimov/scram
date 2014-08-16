/// @file fault_tree.h
/// Fault Tree.
#ifndef SCRAM_FAULT_TREE_H_
#define SCRAM_FAULT_TREE_H_

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <queue>

#include <boost/serialization/map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "error.h"
#include "event.h"
#include "risk_analysis.h"
#include "superset.h"

// class FaultTreeTest;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

typedef boost::shared_ptr<scram::Superset> SupersetPtr;

namespace scram {

/// @class FaultTree
/// Fault tree representation.
class FaultTree {
  // friend class ::FaultTreeTest;

 public:
  /// The main constructor of the Fault Tree.
  /// @param[in] name The name identificator of this fault tree.
  FaultTree(std::string name);

  virtual ~FaultTree() {}

  /// Adds node and updates databases of intermediate and primary events.
  /// @param[in] parent The id of the parent node.
  /// @param[in] id The id name of the node to be created.
  /// @param[in] type The symbol, type, or gate of the event to be added.
  /// @param[in] vote_number The vote number for the VOTE gate initialization.
  /// @throws ValidationError for invalid or incorrect inputs.
  void AddEvent(std::string parent, std::string id, std::string type,
                int vote_number = -1);

  /// New tree methhods.
  void InsertGate(std::string id, std::string type, int vote_number = -1);
  void InsertPrimary(std::string id, std::string type);

  /// Adds probability to a primary event for p-model.
  /// @param[in] id The id name of the primary event.
  /// @param[in] p The probability for the primary event.
  /// @note If id is not in the tree, the probability is ignored.
  void AddProb(std::string id, double p);

  /// Adds probability to a primary event for l-model.
  /// @param[in] id The id name of the primary event.
  /// @param[in] p The probability for the primary event.
  /// @param[in] time The time to failure for this event.
  /// @note If id is not in the tree, the probability is ignored.
  void AddProb(std::string id, double p, double time);

  /// Verifies if gates are initialized correctly.
  /// @returns A warning message with a list of all bad gates with problems.
  /// @note An empty string for no problems detected.
  std::string CheckAllGates();

 private:
  /// Checks if a gate is initialized correctly.
  /// @returns A warning message with the problem description.
  /// @note An empty string for no problems detected.
  std::string CheckGate_(const GatePtr& event);

  /// @returns Primary events that do not have probabilities assigned.
  /// @note An empty string for no problems detected.
  std::string PrimariesNoProb_();

  // ----------------------- Member Variables of this Class -----------------
  /// The name of this fault tree.
  std::string name_;
  /// This member is used to provide any warnings about assumptions,
  /// calculations, and settings. These warnings must be written into output
  /// file.
  std::string warnings_;

  /// Input file path.
  std::string input_file_;

  /// Keep track of currently opened file with sub-trees.
  std::string current_file_;

  /// Container of original names of events with capitalizations.
  std::map<std::string, std::string> orig_ids_;

  /// List of all valid gates.
  std::set<std::string> gates_;

  /// List of all valid types of primary events.
  std::set<std::string> types_;

  /// Id of a top event.
  std::string top_event_id_;

  /// Top event.
  GatePtr top_event_;

  /// Indicator of detection of a top event described by a transfer sub-tree.
  bool top_detected_;

  /// Indicates that reading the main tree file as opposed to a transfer tree.
  bool is_main_;

  /// Holder for intermediate events.
  boost::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Container for transfer symbols as requested in tree initialization.
  /// A queue contains a tuple of the parent and id of transferIn.
  std::queue< std::pair<std::string, std::string> > transfers_;

  /// For graphing purposes the same transferIn.
  std::multimap<std::string, std::string> transfer_map_;

  /// Container for storing all transfer sub-trees' names and number of calls.
  std::map<std::string, int> trans_calls_;

  /// Container to track transfer calls to prevent cyclic calls/inclusions.
  std::map< std::string, std::vector<std::string> > trans_tree_;
};

}  // namespace scram

#endif  // SCRAM_FAULT_TREE_H_
