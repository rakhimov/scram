/// @file top_event.cc
/// Implementation of top event class.
#include "event.h"

#include "error.h"

namespace scram {

TopEvent::TopEvent(std::string id, std::string gate)
    : scram::Event(id),
      gate_(gate),
      vote_number_(-1) {}

const std::string& TopEvent::gate() {
  if (gate_ == "NONE") {
    std::string msg = "Gate is not set for " + this->id() + " event.";
    throw scram::ValueError(msg);
  }
  return gate_;
}

void TopEvent::gate(std::string gate) {
  if (gate_ != "NONE") {
    std::string msg = "Trying to re-assign a gate for " + this->id() +
                      " event.";
    throw scram::ValueError(msg);
  }
  gate_ = gate;
}

int TopEvent::vote_number() {
  if (vote_number_ == -1) {
    std::string msg = "Vote number is not set for " + this->id() + " event.";
    throw scram::ValueError(msg);
  }
  return vote_number_;
}

void TopEvent::vote_number(int vnumber) {
  if (gate_ != "vote") {
    // This line calls gate() function which may throw an exception if
    // the gate of this event is not yet set.
    std::string msg = "Vote number can only be defined for the VOTE gate. "
                      "The " + this->id() + " event has " + this->gate() + ".";
    throw scram::ValueError(msg);
  } else if (vnumber < 2) {
    std::string msg = "Vote number cannot be less than 2.";
    throw scram::ValueError(msg);
  } else if (vote_number_ != -1) {
    std::string msg = "Trying to re-assign a vote number for " + this->id() +
                      " event.";
    throw scram::ValueError(msg);
  }  // Children number should be checked outside of this class.
  vote_number_ = vnumber;
}

const std::map<std::string,
               boost::shared_ptr<scram::Event> >& TopEvent::children() {
  if (children_.empty()) {
    std::string msg = this->id() + " event does not have children.";
    throw scram::ValueError(msg);
  }
  return children_;
}

void TopEvent::AddChild(const boost::shared_ptr<scram::Event>& child) {
  if (children_.count(child->id())) {
    std::string msg = "Trying to re-insert a child for " + this->id() +
                      " event.";
    throw scram::ValueError(msg);
  }
  children_.insert(std::make_pair(child->id(), child));
}

}  // namespace scram
