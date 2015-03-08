/// @file fault_tree.h
/// Fault Tree.
#ifndef SCRAM_SRC_FAULT_TREE_H_
#define SCRAM_SRC_FAULT_TREE_H_

#include <string>
#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "element.h"
#include "event.h"

namespace scram {

/// @class FaultTree
/// Fault tree representation.
class FaultTree : public Element {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;

  /// The main constructor of the Fault Tree.
  /// @param[in] name The name identificator of this fault tree.
  explicit FaultTree(std::string name);

  /// Adds a gate into this tree.
  /// The first gate is assumed to be a top event.
  /// @param[in] gate The gate to be added to this tree.
  /// @throws ValidationError for readded gates and out-of-order addition.
  void AddGate(const GatePtr& gate);

  /// Validates this tree's structure and events.
  /// Checks the tree for cyclicity.
  /// @throws ValidationError if there are issues with this tree.
  /// @note This is an expensive function, but it must be called at least
  /// once after finilizing fault tree instantiation.
  void Validate();

  /// Gathers information about the initialized fault tree. Databases
  /// for events are manipulated to best reflect the state and structure
  /// of the fault tree. This function must be called after validation.
  /// This function must be called before any analysis is performed because
  /// there would not be necessary information available for analysis like
  /// primary events of this fault tree. Moreover, all the nodes of this
  /// fault tree are expected to be defined fully and correctly.
  /// @throws LogicError if the fault tree is not fully defined or some
  ///                    information is mission.
  void SetupForAnalysis();

  /// @returns The name of this tree.
  inline const std::string& name() { return name_; }

  /// @returns The top gate.
  inline GatePtr& top_event() { return top_event_; }

  /// @returns The container of intermediate events.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, GatePtr>& inter_events() {
    return inter_events_;
  }

  /// @returns The container of intermediate events that are defined implicitly
  ///          by traversing the tree instead of initiating AddGate() function.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, GatePtr>& implicit_gates() {
    return implicit_gates_;
  }

  /// @returns The container of primary events of this tree.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, PrimaryEventPtr>&
      primary_events() {
    return primary_events_;
  }

  /// @returns The container of all basic events of this tree. This includes
  ///          basic events created to represent common cause failure.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      basic_events() {
    return basic_events_;
  }

  /// @returns Basic events that are in some CCF groups.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      ccf_events() {
    return ccf_events_;
  }

  /// @returns The container of house events of this tree.
  /// @warning The tree must be validated and ready for analysis.
  inline const boost::unordered_map<std::string, HouseEventPtr>&
      house_events() {
    return house_events_;
  }

  /// @returns The original number of basic events without new CCF basic events.
  /// @warning The tree must be validated and ready for analysis.
  inline int num_basic_events() { return num_basic_events_; }

 private:
  typedef boost::shared_ptr<Event> EventPtr;

  /// Traverses the tree to find any cyclicity.
  /// While traversing, this function observes implicitly defined gates, and
  /// those gates are added into the gate containers.
  /// @param[in] parent The gate to start with.
  /// @param[in] path The current path from the start gate.
  /// @param[in] visited The gates that are already visited in the path.
  void CheckCyclicity(const GatePtr& parent, std::vector<std::string> path,
                      std::set<std::string> visited);

  /// Picks primary events of this tree.
  /// Populates the container of primary events.
  void GatherPrimaryEvents();

  /// Picks primary events of the specified gate.
  /// The primary events are put into the approriate container.
  /// @param[in] gate The gate to get primary events from.
  void GetPrimaryEvents(const GatePtr& gate);

  /// Picks basic events created by CCF groups.
  /// Populates the container of basic and primary events.
  void GatherCcfBasicEvents();

  /// The name of this fault tree.
  std::string name_;

  /// Id of a top event.
  std::string top_event_id_;

  /// Top event.
  GatePtr top_event_;

  /// Holder for intermediate events.
  boost::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for primary events of the tree.
  /// This container is filled implicitly by traversing the tree.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Container for basic events of the tree.
  /// This container is filled implicitly by traversing the tree.
  /// In addition, common cause failure basic events are included.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for basic events that are identified to be in some CCF group.
  /// These basic events are not necessarily in the same CCF group.
  boost::unordered_map<std::string, BasicEventPtr> ccf_events_;

  /// Container for house events of the tree.
  /// This container is filled implicitly by traversing the tree.
  boost::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Implicitly added gates.
  /// This gates are not added through AddGate() function but by traversing
  /// the tree as a postprocess.
  boost::unordered_map<std::string, GatePtr> implicit_gates_;

  /// The number of original basic events without new CCF basic events.
  int num_basic_events_;
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_H_
