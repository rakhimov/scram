/// @file fault_tree.h
/// Fault Tree container.
#ifndef SCRAM_SRC_FAULT_TREE_H_
#define SCRAM_SRC_FAULT_TREE_H_

#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "element.h"
#include "event.h"

namespace scram {

class CcfGroup;
class Component;
class Parameter;

/// @class FaultTree
/// Fault tree representation as a container of gates, basic and house events,
/// and other information.
class FaultTree : public Element {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<Parameter> ParameterPtr;
  typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;
  typedef boost::shared_ptr<Component> ComponentPtr;

  /// The main constructor of the Fault Tree.
  /// @param[in] name The name identificator of this fault tree.
  explicit FaultTree(std::string name);

  virtual ~FaultTree() {}

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

  /// Adds a parameter into this fault tree containter.
  /// @param[in] parameter The parameter to be added to this tree.
  /// @throws ValidationError for re-added parameter.
  void AddParameter(const ParameterPtr& parameter);

  /// Adds a ccf group and its members into this fault tree containter.
  /// @param[in] ccf_group The ccf group to be added to this container.
  /// @throws ValidationError for re-added ccf groups or duplicate basic event
  ///         members.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

  /// Adds a component container into this fault tree containter.
  /// @param[in] component The ccf group to be added to this container.
  /// @throws ValidationError for re-added components.
  void AddComponent(const ComponentPtr& component);

  /// Validates this fault tree's structure and events.
  /// This step must be called before any other function that requests member
  /// containers of top events, gates, basic events, house events, and so on.
  /// @throws ValidationError if there are issues with this fault tree.
  void Validate();

  /// @returns The name of this fault tree.
  inline const std::string& name() const { return name_; }

  /// @returns The top events of this fault tree.
  inline const std::vector<GatePtr>& top_events() const { return top_events_; }

  /// @returns The container of all gates of this fault tree with
  ///          lower-case names as keys.
  inline const boost::unordered_map<std::string, GatePtr>& gates() const {
    return gates_;
  }

  /// @returns The container of all basic events of this fault tree with
  ///          lower-case names as keys.
  inline const boost::unordered_map<std::string, BasicEventPtr>&
      basic_events() const {
    return basic_events_;
  }

  /// @returns The container of house events of this fault tree with lower-case
  ///          names as keys.
  inline const boost::unordered_map<std::string, HouseEventPtr>&
      house_events() const {
    return house_events_;
  }

  /// @returns The container of parameters of this fault tree with lower-case
  ///          names as keys.
  inline const boost::unordered_map<std::string, ParameterPtr>&
      parameters() const {
    return parameters_;
  }

  /// @returns CCF groups belonging to this fault tree with lower-case names as
  ///          keys.
  inline const boost::unordered_map<std::string, CcfGroupPtr>&
      ccf_groups() const {
    return ccf_groups_;
  }

  /// @returns Components in this fault tree container with lower-case names as
  ///          keys.
  inline const boost::unordered_map<std::string, ComponentPtr>&
      components() const {
    return components_;
  }

 protected:
  /// Recusively traverses components to gather gates relevant to
  /// the whole fault tree.
  /// @param[out] gates Gates belonging to this component and its subcomponents.
  void GatherGates(boost::unordered_set<GatePtr>* gates);

 private:
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Recursively marks descendant gates as "non-top". These gates belong
  /// to this fault tree only.
  /// @param[in] gate The ancestor gate.
  /// @param[in] gates Gates belonging to the whole fault tree with components.
  void MarkNonTopGates(const GatePtr& gate,
                       const boost::unordered_set<GatePtr>& gates);

  /// Recursively marks descendant gates in formulas as "non-top"
  /// @param[in] formula The formula of a gate or another formula.
  /// @param[in] gates Gates belonging to the whole fault tree with components.
  void MarkNonTopGates(const FormulaPtr& formula,
                       const boost::unordered_set<GatePtr>& gates);

  std::string name_;  ///< The name of this fault tree.
  std::vector<GatePtr> top_events_;  ///< Top events of this fault tree.

  /// Container for gates with lower-case names as keys.
  boost::unordered_map<std::string, GatePtr> gates_;

  /// Container for basic events with lower-case names as keys.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for house events with lower-case names as keys.
  boost::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for parameters with lower-case names as keys.
  boost::unordered_map<std::string, ParameterPtr> parameters_;

  /// Container for CCF groups with lower-case names as keys.
  boost::unordered_map<std::string, CcfGroupPtr> ccf_groups_;

  /// Container for components with lower-case names as keys.
  boost::unordered_map<std::string, ComponentPtr> components_;
};

/// @class Component
/// Component is for logical grouping of events, gates, and other components.
class Component : public FaultTree, public Role {
 public:
  /// Constructs a component assuming that exists within some fault tree.
  /// The public or private role of a component is not for the components
  /// itself, but for the events and parameters of the component. Component name
  /// is not meant to be public; however, it must be unique with the parent
  /// fault tree or component.
  /// @param[in] name The name identificator for the component.
  /// @param[in] base_path The series of containers to get this container.
  /// @param[in] is_public A flag to define public or private role for members.
  explicit Component(const std::string& name, const std::string& base_path = "",
                     bool is_public = true);
};

}  // namespace scram

#endif  // SCRAM_SRC_FAULT_TREE_H_
