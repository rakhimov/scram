/// @file event.cc
/// Implementation of Event Class.
#include "event.h"

namespace scram {

Event::Event(std::string id, std::string orig_id)
    : id_(id),
      orig_id_(orig_id) {}

void Event::AddParent(const boost::shared_ptr<Gate>& parent) {
  if (parents_.count(parent->id())) {
    std::string msg = "Trying to re-insert existing parent for " +
                      this->orig_id();
    throw LogicError(msg);
  }
  parents_.insert(std::make_pair(parent->id(), parent));
}

const std::map<std::string, boost::shared_ptr<Gate> >& Event::parents() {
  if (parents_.empty()) {
    std::string msg = this->orig_id() + " does not have parents.";
    throw LogicError(msg);
  }
  return parents_;
}

Gate::Gate(std::string id, std::string type)
    : Event(id),
      type_(type),
      vote_number_(-1),
      mark_("") {}

const std::string& Gate::type() {
  if (type_ == "NONE") {
    std::string msg = "Gate type is not set for " + this->orig_id() + " gate.";
    throw LogicError(msg);
  }
  return type_;
}

void Gate::type(std::string type) {
  if (type_ != "NONE") {
    std::string msg = "Trying to re-assign a gate type for " +
                      this->orig_id() + " gate.";
    throw LogicError(msg);
  }
  type_ = type;
}

int Gate::vote_number() {
  if (vote_number_ == -1) {
    std::string msg = "Vote number is not set for " +
                      this->orig_id() + " gate.";
    throw LogicError(msg);
  }
  return vote_number_;
}

void Gate::vote_number(int vnumber) {
  if (type_ != "atleast") {
    // This line calls type() function which may throw an exception if
    // the type of this gate is not yet set.
    std::string msg = "Vote number can only be defined for the ATLEAST gate. "
                      "The " + this->orig_id() + " gate is " +
                      this->type() + ".";
    throw LogicError(msg);
  } else if (vnumber < 2) {
    std::string msg = "Vote number cannot be less than 2.";
    throw InvalidArgument(msg);
  } else if (vote_number_ != -1) {
    std::string msg = "Trying to re-assign a vote number for " +
                      this->orig_id() + " gate.";
    throw LogicError(msg);
  }
  vote_number_ = vnumber;
}

void Gate::AddChild(const boost::shared_ptr<Event>& child) {
  if (children_.count(child->id())) {
    std::string msg = "Trying to re-insert a child for " + this->orig_id() +
                      " gate.";
    throw LogicError(msg);
  }
  children_.insert(std::make_pair(child->id(), child));
}

const std::map<std::string, boost::shared_ptr<Event> >& Gate::children() {
  if (children_.empty()) {
    std::string msg = this->orig_id() + " gate does not have children.";
    throw LogicError(msg);
  }
  return children_;
}

}  // namespace scram
