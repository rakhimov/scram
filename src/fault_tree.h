/// @file fault_tree.h
/// Fault Tree.
#ifndef SCRAM_SRC_FAULT_TREE_H_
#define SCRAM_SRC_FAULT_TREE_H_

#include <map>
#include <string>
#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "element.h"
#include "event.h"

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

namespace scram {

/// @class FaultTree
/// Fault tree representation.
class FaultTree : public Element {
 public:
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
  /// Populates necessary primary event and gate container.
  /// @throws ValidationError if there are issues with this tree.
  /// @note This is expensive function, but it must be called at least
  /// once after finilizing fault tree instantiation.
  void Validate();

  /// @returns The name of this tree.
  inline const std::string& name() { return name_; }

  /// @returns The top gate.
  inline GatePtr& top_event() { return top_event_; }

  /// @returns The container of intermediate events.
  /// @warning Validate function must be called before this function.
  inline const boost::unordered_map<std::string, GatePtr>& inter_events() {
    return inter_events_;
  }

  /// @returns The container of intermediate events that are defined implicitly
  ///          by traversing the tree instead of initiating AddGate() function.
  /// @warning Validate function must be called before this function.
  inline const boost::unordered_map<std::string, GatePtr>& implicit_gates() {
    return implicit_gates_;
  }

  /// @returns The container of primary events of this tree.
  /// @warning Validate function must be called before this function.
  inline const boost::unordered_map<std::string, PrimaryEventPtr>&
      primary_events() {
    return primary_events_;
  }

 private:
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

  /// The name of this fault tree.
  std::string name_;

  /// Id of a top event.
  std::string top_event_id_;

  /// Top event.
  GatePtr top_event_;

  /// Holder for intermediate events.
  boost::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for the primary events of the tree.
  /// This container is filled implicitly by traversing the tree.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Implicitly added gates.
  /// This gates are not added through AddGate() function but by traversing
  /// the tree as a postprocess.
  boost::unordered_map<std::string, GatePtr> implicit_gates_;
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_H_
