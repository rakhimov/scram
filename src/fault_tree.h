/// @file fault_tree.h
/// Fault Tree container.
#ifndef SCRAM_SRC_FAULT_TREE_H_
#define SCRAM_SRC_FAULT_TREE_H_

#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "element.h"
#include "event.h"

namespace scram {

class CcfGroup;

/// @class FaultTree
/// Fault tree representation as a container of gates, basic and house events,
/// and other information.
class FaultTree : public Element {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;

  /// The main constructor of the Fault Tree.
  /// @param[in] name The name identificator of this fault tree.
  explicit FaultTree(std::string name);

  /// Adds a gate into this fault tree container.
  /// @param[in] gate The gate to be added to this tree.
  /// @throws ValidationError for re-added gates.
  void AddGate(const GatePtr& gate);

  /// Adds a basic event into this fault tree containter.
  /// @param[in] basic_event The basic event to be added to this tree.
  /// @throws ValidationError for re-added basic events.
  void AddBasicEvent(const BasicEventPtr& basic_event);

  /// Adds a house event into this fault tree containter.
  /// @param[in] house_event The house event to be added to this tree.
  /// @throws ValidationError for re-added house events.
  void AddHouseEvent(const HouseEventPtr& house_event);

  /// Adds a ccf group into this fault tree containter.
  /// @param[in] ccf_group The ccf group to be added to this container.
  /// @throws ValidationError for re-added ccf groups.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

  /// Validates this fault tree's structure and events.
  /// This step must be called before any other function that requests member
  /// containers of top events, gates, basic events, house events, and so on.
  /// @throws ValidationError if there are issues with this fault tree.
  void Validate();

  /// @returns The name of this fault tree.
  inline const std::string& name() { return name_; }

  /// @returns The top events of this fault tree.
  inline const std::vector<GatePtr>& top_events() { return top_events_; }

  /// @returns The container of all basic events of this fault tree.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      basic_events() {
    return basic_events_;
  }

  /// @returns The container of house events of this fault tree.
  inline const boost::unordered_map<std::string, HouseEventPtr>&
      house_events() {
    return house_events_;
  }

  /// @returns CCF groups belonging to this fault tree.
  inline const boost::unordered_map<std::string, CcfGroupPtr>&
      ccf_groups() {
    return ccf_groups_;
  }

 private:
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Recursively marks descendant gates as "non-top". These gates belong
  /// to this fault tree only.
  /// @param[in] gate The ancestor gate.
  void MarkNonTopGates(const GatePtr& gate);

  /// Recursively marks descendant gates in formulas as "non-top"
  /// @param[in] formula The formula of a gate or another formula.
  void MarkNonTopGates(const FormulaPtr& formula);

  /// Holder for gates defined in this fault tree container.
  boost::unordered_map<std::string, GatePtr> gates_;

  std::string name_;  ///< The name of this fault tree.
  std::vector<GatePtr> top_events_;  ///< Top events of this fault tree.

  /// Container for basic events of the tree.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for house events of the tree.
  boost::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for CCF groups.
  boost::unordered_map<std::string, CcfGroupPtr> ccf_groups_;
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_H_
