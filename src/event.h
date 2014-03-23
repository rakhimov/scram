#ifndef SCRAM_EVENT_H_
#define SCRAM_EVENT_H_

#include <map>
#include <string>
#include <vector>

namespace scram {

// General event base class
class Event {
 public:
  Event(std::string id) : id_(id) {}

  virtual std::string id() {
    return id_;
  }

  virtual ~Event() {}

 private:
  // Id name of an event
  std::string id_;
};

// Top event
// This class should not have a parent
class TopEvent : public scram::Event {
 public:
  // Constructs with a gate
  TopEvent(std::string id, std::string gate = "NONE");

  // Returns the gate type
  virtual std::string gate();

  // Sets the gate type
  virtual void gate(std::string gate);

  // Returns children
  virtual const std::vector<scram::Event*>& children();

  // Adds a child into children list
  virtual void add_child(scram::Event* child);

  virtual ~TopEvent() {}

 private:
  // Gate type
  std::string gate_;
  // Intermidiate and basic events of the top event
  std::vector<scram::Event*> children_;
};

// Intermidiate event
// This event is Top event with a parent
class InterEvent : public scram::TopEvent {
 public:
  // Constructs with id, gate, and parent
  InterEvent(std::string id, std::string gate = "NONE",
             scram::Event* parent = NULL);

  // Returns the parent
  const scram::Event* parent();

  // Sets the parent
  void parent(scram::Event* parent);

  ~InterEvent() {}

 private:
  // The parent of this intermidiate event
  scram::Event* parent_;

};

// Basic event
// This class should not have childrens
class BasicEvent : public scram::Event {
 public:
  // Constructs with id name and probability
  BasicEvent(std::string id, double p = -1);

  // Returns the probability
  double p();

  // Sets the probability
  void p(double p);

  // Adds a parent into the parent map
  void add_parent(scram::Event* parent);

  // Return parents
  std::map<std::string, scram::Event*>& parents();

  ~BasicEvent() {}

 private:
  // Probability of the basic event
  double p_;

  // Parents of this basic event
  std::map<std::string, scram::Event*> parents_;
};

}  // namespace scram

#endif  // SCRAM_EVENT_H_
