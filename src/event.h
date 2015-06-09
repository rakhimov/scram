/// @file event.h
/// Contains event classes for fault trees.
#ifndef SCRAM_SRC_EVENT_H_
#define SCRAM_SRC_EVENT_H_

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "element.h"
#include "error.h"
#include "expression.h"

namespace scram {

class Gate;  // Needed for being a parent of an event.

/// @class Event
/// General fault tree event base class.
class Event : public Element {
 public:
  /// Constructs a fault tree event with a specific id.
  /// @param[in] id The identifying name for the event.
  /// @param[in] name The identifying name with caps preserved.
  explicit Event(std::string id, std::string name = "");

  virtual ~Event() {}

  /// @returns The id that is set upon the construction of this event.
  inline const std::string& id() const { return id_; }

  /// @returns The original name with capitalizations.
  inline const std::string& name() const { return name_; }

  /// Sets the original name with capitalizations preserved.
  /// @param[in] id_with_caps The id name with capitalizations.
  void name(std::string id_with_caps) { name_ = id_with_caps; }

  /// Adds a parent into the parent map.
  /// @param[in] parent One of the gate parents of this event.
  /// @throws LogicError if the parent is being re-inserted.
  void AddParent(const boost::shared_ptr<Gate>& parent);

  /// @returns All the parents of this gate event.
  /// @throws LogicError if there are no parents for this gate event.
  const std::map<std::string, boost::shared_ptr<Gate> >& parents();

  /// @returns True if this event is orphan.
  inline bool IsOrphan() { return parents_.empty(); }

 private:
  std::string id_;  ///< Id name of a event. It is in lower case.
  std::string name_;  ///< Original name with capitalizations preserved.
  ///< The parents of this event.
  std::map<std::string, boost::shared_ptr<Gate> > parents_;
};

/// @class Gate
/// A representation of a gate in a fault tree.
class Gate : public Event {
 public:
  /// Constructs with an id and a gate.
  /// @param[in] id The identifying name for this event.
  /// @param[in] type The type for this gate.
  explicit Gate(std::string id, std::string type = "NONE");

  /// @returns The gate type.
  /// @throws LogicError if the gate is not yet assigned.
  const std::string& type();

  /// Sets the gate type.
  /// @param[in] type The gate type for this event.
  /// @throws LogicError if the gate type is being re-assigned.
  void type(std::string type);

  /// @returns The vote number if and only if the gate is vote.
  /// @throws LogicError if the vote number is not yet assigned.
  int vote_number();

  /// Sets the vote number only for a vote gate.
  /// @param[in] vnumber The vote number.
  /// @throws InvalidArgument if the vote number is invalid.
  /// @throws LogicError if the vote number is assigned illegally.
  /// @note (Children number > vote number)should be checked outside of
  ///        this class.
  void vote_number(int vnumber);

  /// Adds a child event into the children list.
  /// @param[in] child A pointer to a child event.
  /// @throws LogicError if the child is being re-inserted.
  void AddChild(const boost::shared_ptr<Event>& child);

  /// @returns The children of this gate.
  /// @throws LogicError if there are no children, which should be checked
  ///                    at gate initialization.
  const std::map<std::string, boost::shared_ptr<Event> >& children();

  /// Checks if a gate is initialized correctly.
  /// @throws Validation error if anything is wrong.
  void Validate();

  /// This function is for cycle detection.
  /// @returns The connector between gates.
  inline Gate* connector() { return this; }

  /// @returns The mark of this gate node. Empty string for no mark.
  inline const std::string& mark() const { return mark_; }

  /// Sets the mark for this gate node.
  inline void mark(const std::string& new_mark) { mark_ = new_mark; }

  /// @returns Parameters as nodes.
  inline const std::vector<Gate*>& nodes() {
    if (gather_) Gate::GatherNodesAndConnectors();
    return nodes_;
  }

  /// @returns Non-Parameter Expressions as connectors.
  inline const std::vector<Gate*>& connectors() {
    if (gather_) Gate::GatherNodesAndConnectors();
    return connectors_;
  }

 private:
  /// Gathers nodes and connectors from children of the gate.
  void GatherNodesAndConnectors();

  /// Checks if an Inhibit gate is initialized correctly.
  /// @returns A warning message with the problem description.
  /// @returns An empty string for no problems detected.
  std::string CheckInhibitGate();

  std::string type_;  ///< Gate type.
  int vote_number_;  ///< Vote number for the vote gate.
  std::string mark_;  ///< The mark for traversal or toposort.
  /// The children of this gate.
  std::map<std::string, boost::shared_ptr<Event> > children_;
  std::vector<Gate*> nodes_;  ///< Gate children as nodes.
  std::vector<Gate*> connectors_;  ///< Formulae as connectors.
  bool gather_;  ///< A flag to gather nodes and connectors.
};

/// @class PrimaryEvent
/// This is a base class for events that can cause faults.
/// This class represents Base, House, Undeveloped, and other events.
class PrimaryEvent : public Event {
 public:
  /// Constructs with id name and probability.
  /// @param[in] id The identifying name of this primary event.
  /// @param[in] type The type of the event.
  explicit PrimaryEvent(std::string id, std::string type = "")
      : type_(type),
        has_expression_(false),
        Event(id) {}

  virtual ~PrimaryEvent() {}

  /// @returns The type of the primary event.
  inline const std::string& type() const { return type_; }

  /// @returns A flag indicating if the event's expression is set.
  inline bool has_expression() const { return has_expression_; }

 protected:
  /// Flag to notify that expression for the event is defined.
  bool has_expression_;

 private:
  std::string type_;  ///< The type of the primary event.
};

/// @class BasicEvent
/// Representation of a basic event in a fault tree.
class BasicEvent : public PrimaryEvent {
 public:
  typedef boost::shared_ptr<Expression> ExpressionPtr;

  /// Constructs with id name.
  /// @param[in] id The identifying name of this basic event.
  explicit BasicEvent(std::string id) : PrimaryEvent(id, "basic") {}

  virtual ~BasicEvent() {}

  /// Sets the expression of this basic event.
  /// @param[in] expression The expression to describe this event.
  inline void expression(const ExpressionPtr& expression) {
    assert(!expression_);
    PrimaryEvent::has_expression_ = true;
    expression_ = expression;
  }

  /// @returns The mean probability of this basic event.
  /// @note The user of this function should make sure that the returned
  ///       value is acceptable for calculations.
  /// @warning Undefined behavior if the expression is not set.
  inline double p() const {
    assert(expression_);
    return expression_->Mean();
  }

  /// Samples probability value from its probability distribution.
  /// @returns Sampled value.
  /// @note The user of this function should make sure that the returned
  ///       value is acceptable for calculations.
  /// @warning Undefined behavior if the expression is not set.
  inline double SampleProbability() {
    assert(expression_);
    return expression_->Sample();
  }

  /// Resets the sampling.
  inline void Reset() { expression_->Reset(); }

  /// @returns Indication if this event does not have uncertainty.
  inline bool IsConstant() { return expression_->IsConstant(); }

  /// Validates the probability expressions for the primary event.
  /// @throws Validation error if anything is wrong.
  void Validate() {
    if (expression_->Min() < 0 || expression_->Max() > 1) {
      throw ValidationError("Expression value is invalid.");
    }
  }

  /// Indicates if this basic event has been set to be in a CCF group.
  /// @returns true if in a CCF group.
  /// @returns false otherwise.
  bool HasCcf() { return ccf_gate_ ? true : false; }

  /// Sets the common cause failure group gate that can represent this basic
  /// event in analysis with common cause information. This information is
  /// expected to be provided by CCF group application.
  /// @param[in] gate CCF group gate.
  void ccf_gate(const boost::shared_ptr<Gate>& gate) {
    assert(!ccf_gate_);
    ccf_gate_ = gate;
  }

  /// @returns CCF group gate representing this basic event.
  const boost::shared_ptr<Gate>& ccf_gate() {
    assert(ccf_gate_);
    return ccf_gate_;
  }

 private:
  /// Expression that describes this basic event and provides numerical
  /// values for probability calculations.
  ExpressionPtr expression_;

  /// If this basic event is in a common cause group, CCF gate can serve
  /// as a replacement for the basic event for common cause analysis.
  boost::shared_ptr<Gate> ccf_gate_;
};

/// @class HouseEvent
/// Representation of a house event in a fault tree.
class HouseEvent : public PrimaryEvent {
 public:
  /// Constructs with id name.
  /// @param[in] id The identifying name of this house event.
  explicit HouseEvent(std::string id)
      : state_(false),
        PrimaryEvent(id, "house") {}

  /// Sets the state for House event.
  /// @param[in] constant False or True for the state of this house event.
  inline void state(bool constant) {
    PrimaryEvent::has_expression_ = true;
    state_ = constant;
  }

  /// @returns The true or false state of this house event.
  inline bool state() const { return state_; }

 private:
  /// Represents the state of the house event.
  /// Implies On or Off for True or False values of the probability.
  bool state_;
};

/// @class CcfEvent
/// A basic event that represents a multiple failure of a group of events due to
/// a common cause. This event is generated out of a common cause group.
/// This class is a helper to report correctly the CCF events.
class CcfEvent : public BasicEvent {
 public:
  /// Constructs CCF event with id name that is used for internal purposes.
  /// This id is formatted by CcfGroup. The original name is also formatted by
  /// CcfGroup, but the original name may not be suitable for reporting.
  /// @param[in] id The identifying name of this CCF event.
  /// @param[in] ccf_group_name The name of CCF group for reporting.
  /// @param[in] ccf_group_size The total size of CCF group for reporting.
  CcfEvent(std::string id, std::string ccf_group_name, int ccf_group_size)
      : BasicEvent(id),
        ccf_group_name_(ccf_group_name),
        ccf_group_size_(ccf_group_size) {}

  /// @returns The name of the original CCF group.
  inline const std::string ccf_group_name() { return ccf_group_name_; }

  /// @returns The total size of the original CCF group.
  inline int ccf_group_size() { return ccf_group_size_; }

  /// @returns Original names of members of this CCF event.
  inline const std::vector<std::string>& member_names() {
    return member_names_;
  }

  /// Sets original names of members.
  /// @param[in] names A container of original names of basic events.
  inline const void member_names(const std::vector<std::string>& names) {
    member_names_ = names;
  }

 private:
  /// The name of the CCF group that this CCF event is constructed from.
  std::string ccf_group_name_;
  int ccf_group_size_;  ///< CCF group size.
  /// Original names of basic events in this CCF event.
  std::vector<std::string> member_names_;
};

}  // namespace scram

#endif  // SCRAM_SRC_EVENT_H_
