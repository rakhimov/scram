/// @file event.h
/// Contains event classes for fault trees.
#ifndef SCRAM_EVENT_H_
#define SCRAM_EVENT_H_

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "error.h"

namespace scram {

class Gate;  // Needed for being a parent of an event.

/// @class Event
/// General fault tree event base class.
class Event {
 public:
  /// Constructs a fault tree event with a specific id.
  /// @param[in] id The identifying name for the event.
  Event(std::string id);

  /// @returns The id that is set upon the construction of this event.
  const std::string& id() { return id_; }

  /// Adds a parent into the parent map.
  /// @param[in] parent One of the parents of this gate event.
  /// @throws ValueError if the parent is being re-inserted.
  void AddParent(const boost::shared_ptr<scram::Gate>& parent);

  /// @returns All the parents of this gate event.
  /// @throws ValueError if there are no parents for this gate event.
  const std::map<std::string, boost::shared_ptr<scram::Gate> >& parents();

  virtual ~Event() {}

 private:
  /// Id name of a event.
  std::string id_;

  /// The parents of this primary event.
  std::map<std::string, boost::shared_ptr<scram::Gate> > parents_;
  /// @todo labels and attributes should be represented here.
};

/// @class Gate
/// A representation of a gate in a fault tree.
class Gate : public scram::Event {
 public:
  /// Constructs with an id and a gate.
  /// @param[in] id The identifying name for this event.
  /// @param[in] type The type for this gate.
  Gate(std::string id, std::string type = "NONE");

  /// @returns The gate type.
  /// @throws ValueError if the gate is not yet assigned.
  const std::string& type();

  /// Sets the gate type.
  /// @param[in] gate The gate type for this event.
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
  virtual void AddChild(const boost::shared_ptr<scram::Event>& child);

  /// @returns The children of this gate.
  /// @throws ValueError if there are no children.
  const std::map<std::string, boost::shared_ptr<scram::Event> >& children();

  ~Gate() {}

 private:
  /// Gate type.
  std::string type_;

  /// Vote number for the vote gate.
  int vote_number_;

  /// The children of this gate.
  std::map<std::string, boost::shared_ptr<scram::Event> > children_;
};

/// @class PrimaryEvent
/// This is a base class for events that can cause faults.
/// This class represents Base, House, Undeveloped, and other events.
class PrimaryEvent : public scram::Event {
 public:
  /// Constructs with id name and probability.
  /// @param[in] id The identifying name of this primary event.
  /// @param[in] type The type of the event.
  /// @param[in] p The failure probability.
  PrimaryEvent(std::string id, std::string type = "", double p = -1);

  /// @returns The type of the primary event.
  /// @throws ValueError if the type is not yet set.
  const std::string& type();

  /// Sets the type.
  /// @param[in] new_type The type for this event.
  /// @throws ValueError if type is not valid or being re-assigned.
  void type(std::string new_type);

  /// @returns The probability of failure of this event.
  /// @throws ValueError if probability is not yet set.
  double p();

  /// Sets the total probability for P-model.
  /// @param[in] p The total failure probability.
  /// @throws ValueError if probability is not a valid value or re-assigned.
  void p(double p);

  /// Sets the total probability for L-model.
  /// @param[in] freq The failure frequency for L-model.
  /// @param[in] time The time to failure for L-model.
  /// @throws ValueError if probability is not a valid value or re-assigned.
  void p(double freq, double time);

  virtual ~PrimaryEvent() {}

 private:
  /// The type of the primary event.
  std::string type_;

  /// The total failure probability of the primary event.
  double p_;
};

/// @class BasicEvent
/// Representation of a basic event in a fault tree.
class BasicEvent: public scram::PrimaryEvent {
 public:
  /// Constructs with id name.
  /// @param[in] id The identifying name of this basic event.
  BasicEvent(std::string id);

  ~BasicEvent() {}

 private:
  /// @todo Probabilities should be moved from Primary event.
};

/// @class HouseEvent
/// Representation of a house event in a fault tree.
class HouseEvent: public scram::PrimaryEvent {
 public:
  /// Constructs with id name.
  /// @param[in] id The identifying name of this basic event.
  HouseEvent(std::string id);

  ~HouseEvent() {}

 private:
  /// @todo Boolean true or false should be represented here.
};

}  // namespace scram

#endif  // SCRAM_EVENT_H_
