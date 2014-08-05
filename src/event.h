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

/// @class Event
/// General event base class.
class Event {
 public:
  /// Constructs an event with a specific id.
  /// @param[in] id The identifying name for this event.
  Event(std::string id) : id_(id) {}

  /// @returns The id that is set upon the construction of this event.
  virtual const std::string& id() { return id_; }

  virtual ~Event() {}

 private:
  /// Id name of an event.
  std::string id_;
};

/// @class TopEvent
/// The top event of a fault tree.
/// @note This class should not have a parent.
class TopEvent : public scram::Event {
 public:
  /// Constructs with an id and a gate.
  /// @param[in] id The identifying name for this event.
  /// @param[in] gate The gate for this event.
  TopEvent(std::string id, std::string gate = "NONE");

  /// @returns The gate type.
  /// @throws ValueError if the gate is not yet assigned.
  virtual const std::string& gate();

  /// Sets the gate type.
  /// @param[in] gate The gate for this event.
  /// @throws ValueError if the gate is being re-assigned.
  virtual void gate(std::string gate);

  /// @returns The vote number iff the gate is vote.
  virtual int vote_number();

  /// Sets the vote number only for a vote gate.
  /// @param[in] vnumber The vote number.
  /// @throws ValueError if the vote number is invalid or being re-assigned.
  virtual void vote_number(int vnumber);

  /// @returns The children of this event.
  /// @throws ValueError if there are no children.
  virtual const std::map<std::string,
                         boost::shared_ptr<scram::Event> >& children();

  /// Adds a child event into the children list.
  /// @param[in] child A pointer to a child event.
  /// @throws ValueError if the child is being re-inserted.
  virtual void AddChild(const boost::shared_ptr<scram::Event>& child);

  virtual ~TopEvent() {}

 private:
  /// Gate type.
  std::string gate_;

  /// Vote number for the vote gate.
  int vote_number_;

  /// Intermediate and primary child events of this top event.
  std::map<std::string, boost::shared_ptr<scram::Event> > children_;
};

/// @class InterEvent
/// The intermediate event for a fault tree.
/// This event is Top event with a parent.
class InterEvent : public scram::TopEvent {
 public:
  /// Constructs with a specific id and gate.
  /// @param[in] id The identifying name for this event.
  /// @param[in] gate The gate for this event.
  InterEvent(std::string id, std::string gate = "NONE");

  /// @returns The parent, which can only be a Top or Intermediate event.
  /// @throws ValueError if the parent is not yet set.
  const boost::shared_ptr<scram::TopEvent>& parent();

  /// Sets the parent.
  /// @param[in] parent The only parent of this intermediate event.
  /// @throws ValueError if the parent is being re-assigned.
  void parent(const boost::shared_ptr<scram::TopEvent>& parent);

  ~InterEvent() {}

 private:
  /// The parent of this intermediate event.
  boost::shared_ptr<scram::TopEvent> parent_;
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

  /// Adds a parent into the parent map.
  /// @param[in] parent One of the parents of this primary event.
  /// @throws ValueError if the parent is being re-inserted.
  void AddParent(const boost::shared_ptr<scram::TopEvent>& parent);

  /// @returns All the parents of this primary event.
  /// @throws ValueError if there are no parents for this primary event.
  std::map<std::string, boost::shared_ptr<scram::TopEvent> >& parents();

  ~PrimaryEvent() {}

 private:
  /// The type of the primary event.
  std::string type_;

  /// The total failure probability of the primary event.
  double p_;

  /// The parents of this primary event.
  std::map<std::string, boost::shared_ptr<scram::TopEvent> > parents_;
};

}  // namespace scram

#endif  // SCRAM_EVENT_H_
