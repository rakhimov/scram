#ifndef SCRAM_EVENT_H_
#define SCRAM_EVENT_H_

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "error.h"

namespace scram {

// General event base class.
class Event {
 public:
  Event(std::string id) : id_(id) {}

  virtual std::string id() {
    return id_;
  }

  virtual ~Event() {}

 private:
  // Id name of an event.
  std::string id_;
};

// Top event.
// This class should not have a parent.
class TopEvent : public scram::Event {
 public:
  // Constructs with a gate.
  TopEvent(std::string id, std::string gate = "NONE");

  // Returns the gate type.
  virtual std::string gate();

  // Sets the gate type.
  virtual void gate(std::string gate);

  // Returns children.
  virtual const std::map<std::string, boost::shared_ptr<scram::Event> >&
      children();

  // Adds a child into children list.
  virtual void AddChild(boost::shared_ptr<scram::Event> child);

  virtual ~TopEvent() {}

 private:
  // Gate type.
  std::string gate_;
  // Intermediate and primary events of the top event.
  std::map<std::string, boost::shared_ptr<scram::Event> > children_;
};

// Intermediate event.
// This event is Top event with a parent.
class InterEvent : public scram::TopEvent {
 public:
  // Constructs with id, gate, and parent.
  InterEvent(std::string id, std::string gate = "NONE");

  // Returns the parent.
  boost::shared_ptr<scram::Event> parent();

  // Sets the parent.
  void parent(boost::shared_ptr<scram::Event> parent);

  ~InterEvent() {}

 private:
  // The parent of this intermediate event.
  boost::shared_ptr<scram::Event> parent_;
};

// This is a base class for events that can cause faults.
// Base, House, Undeveloped, and other events.
class PrimaryEvent : public scram::Event {
 public:
  // Constructs with id name and probability.
  PrimaryEvent(std::string id, std::string type = "", double p = -1,
               std::string p_model = "p-model");

  // Returns the type of the primary event.
  // Throws error if type is not yet set.
  std::string type();

  // Sets the type.
  // Throws error if type is not valid or being re-assigned.
  void type(std::string new_type);

  // Returns the probability.
  // Throws error if probability is not yet set.
  double p();

  // Sets the probability.
  // Throws error if probability is not a valid value.
  void p(double p);

  // Adds a parent into the parent map.
  void AddParent(boost::shared_ptr<scram::Event> parent);

  // Return parents.
  std::map<std::string, boost::shared_ptr<scram::Event> >& parents();

  ~PrimaryEvent() {}

 private:
  // Type of a primary event.
  std::string type_;

  // Probability of the primary event.
  double p_;

  // Probability model.
  std::string p_model_;

  // Parents of this primary event.
  std::map<std::string, boost::shared_ptr<scram::Event> > parents_;
};

}  // namespace scram

#endif  // SCRAM_EVENT_H_
