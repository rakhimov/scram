/// @file fault_tree.h
/// Fault Tree.
#ifndef SCRAM_FAULT_TREE_H_
#define SCRAM_FAULT_TREE_H_

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "error.h"
#include "event.h"

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

namespace scram {

/// @class FaultTree
/// Fault tree representation.
class FaultTree {
 public:
  /// The main constructor of the Fault Tree.
  /// @param[in] name The name identificator of this fault tree.
  explicit FaultTree(std::string name);

  virtual ~FaultTree() {}

  /// Adds a gate into this tree.
  /// The first gate is assumed to be a top event.
  /// @param[in] gate The gate to be added to this tree.
  void AddGate(const GatePtr& gate);

  /// Validates this tree's structure and events.
  /// Populates necessary primary events container.
  /// @throws ValidationError if there are issues with this tree.
  /// @note This is expensive function, but it must be called at least
  /// once after finilizing fault tree instantiation.
  void Validate();

  /// @returns The name of this tree.
  inline const std::string& name() { return name_; }

  /// @returns The top gate.
  inline const GatePtr& top_event() { return top_event_; }

  /// @returns The container of intermediate events.
  inline const boost::unordered_map<std::string, GatePtr>& inter_events() {
    return inter_events_;
  }

  /// @returns The container of primary events of this tree.
  /// @note Assuming that all events in this tree are defined to be gates or
  /// primary events.
  /// @todo Make this inline. Get rid of GatherPrimaryEvents function.
  inline const boost::unordered_map<std::string, PrimaryEventPtr>&
      primary_events() {
    return primary_events_;
  }

 private:
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
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;
};

}  // namespace scram

#endif  // SCRAM_FAULT_TREE_H_
