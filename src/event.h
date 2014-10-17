/// @file event.h
/// Contains event classes for fault trees.
#ifndef SCRAM_SRC_EVENT_H_
#define SCRAM_SRC_EVENT_H_

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "element.h"
#include "expression.h"
#include "error.h"

typedef boost::shared_ptr<scram::Expression> ExpressionPtr;

namespace scram {

class Gate;  // Needed for being a parent of an event.

/// @class Event
/// General fault tree event base class.
class Event : public Element {
 public:
  /// Constructs a fault tree event with a specific id.
  /// @param[in] id The identifying name for the event.
  /// @param[in] orig_id The identifying name with caps preserved.
  explicit Event(std::string id, std::string orig_id = "");

  /// @returns The id that is set upon the construction of this event.
  inline const std::string& id() { return id_; }

  /// @returns The original id with capitalizations.
  inline const std::string& orig_id() { return orig_id_; }

  /// Sets the original id name with capitalizations preserved.
  /// @param[in] id_with_caps The id name with capitalizations.
  void orig_id(std::string id_with_caps) { orig_id_ = id_with_caps; }

  /// Adds a parent into the parent map.
  /// @param[in] parent One of the gate parents of this event.
  /// @throws ValueError if the parent is being re-inserted.
  void AddParent(const boost::shared_ptr<Gate>& parent);

  /// @returns All the parents of this gate event.
  /// @throws ValueError if there are no parents for this gate event.
  const std::map<std::string, boost::shared_ptr<Gate> >& parents();

  virtual ~Event() {}

 private:
  /// Id name of a event. It is in lower case.
  std::string id_;

  /// Id name with capitalizations preserved of a event.
  std::string orig_id_;

  /// The parents of this primary event.
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
  /// @throws ValueError if the gate is not yet assigned.
  const std::string& type();

  /// Sets the gate type.
  /// @param[in] type The gate type for this event.
  /// @throws ValueError if the gate type is being re-assigned.
  void type(std::string type);

  /// @returns The vote number iff the gate is vote.
  int vote_number();

  /// Sets the vote number only for a vote gate.
  /// @param[in] vnumber The vote number.
  /// @throws ValueError if the vote number is invalid or being re-assigned.
  /// @todo A better representation for varios gates as Vote might be a separte
  /// class.
  void vote_number(int vnumber);

  /// Adds a child event into the children list.
  /// @param[in] child A pointer to a child event.
  /// @throws ValueError if the child is being re-inserted.
  void AddChild(const boost::shared_ptr<Event>& child);

  /// Adds children of another gate to this gate.
  /// If this gate exists as a child then it is removed from the children.
  /// @param[in] gate The gate which children are to be added to this gate.
  void MergeGate(const boost::shared_ptr<Gate>& gate);

  /// @returns The children of this gate.
  /// @throws ValueError if there are no children.
  const std::map<std::string, boost::shared_ptr<Event> >& children();

 private:
  /// Gate type.
  std::string type_;

  /// Vote number for the vote gate.
  int vote_number_;

  /// The children of this gate.
  std::map<std::string, boost::shared_ptr<Event> > children_;
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
        Event(id) {}

  virtual ~PrimaryEvent() {}

  /// @returns The type of the primary event.
  /// @throws ValueError if the type is not yet set.
  inline const std::string& type() const { return type_; }

  /// @returns The mean probability of failure of this event.
  /// @throws IllegalOperation if the base class function is called where
  ///                          a derived class operation is meant.
  virtual double p() const {
    throw IllegalOperation("Primary event is not fully defined.");
  }

  /// Samples probability value from its probability distribution.
  /// @returns Sampled value.
  /// @throws IllegalOperation if the base class function is called where
  ///                          a derived class operation is meant.
  virtual double SampleProbability() {
    throw IllegalOperation("Primary event is not fully defined.");
  }

 private:
  /// The type of the primary event.
  std::string type_;
};

/// @class BasicEvent
/// Representation of a basic event in a fault tree.
class BasicEvent: public PrimaryEvent {
 public:
  /// Constructs with id name.
  /// @param[in] id The identifying name of this basic event.
  explicit BasicEvent(std::string id) : PrimaryEvent(id, "basic") {}

  /// Sets the expression of this basic event.
  /// @param[in] expression The expression to describe this event.
  /// @todo Provide more tests that the expression is valid.
  inline void expression(const ExpressionPtr& expression) {
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

 private:
  /// Expression that describes this basic event and provides numerical
  /// values for probability calculations.
  ExpressionPtr expression_;
};

/// @class HouseEvent
/// Representation of a house event in a fault tree.
class HouseEvent: public PrimaryEvent {
 public:
  /// Constructs with id name.
  /// @param[in] id The identifying name of this basic event.
  explicit HouseEvent(std::string id)
      : state_(false),
        PrimaryEvent(id, "house") {}

  /// Sets the state for House event.
  /// @param[in] constant False or True for the state of this house event.
  inline void state(bool constant) { state_ = constant; }

  /// @returns The mean probability of this basic event.
  inline double p() const { return state_ ? 1 : 0; }

  /// The sample for house event is constant value.
  /// @returns Sampled value.
  inline double SampleProbability() { return state_ ? 1 : 0; }

 private:
  /// Represents the state of the house event.
  /// Implies On or Off for True or False values of the probability.
  bool state_;
};

}  // namespace scram

#endif  // SCRAM_SRC_EVENT_H_
