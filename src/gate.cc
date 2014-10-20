/// @file gate.cc
/// Implementation of Gate class.
#include "event.h"

#include "error.h"

namespace scram {

Gate::Gate(std::string id, std::string type)
    : Event(id),
      type_(type),
      vote_number_(-1) {}

const std::string& Gate::type() {
  if (type_ == "NONE") {
    std::string msg = "Gate type is not set for " + this->orig_id() + " gate.";
    throw ValueError(msg);
  }
  return type_;
}

void Gate::type(std::string type) {
  if (type_ != "NONE") {
    std::string msg = "Trying to re-assign a gate type for " +
                      this->orig_id() + " gate.";
    throw ValueError(msg);
  }
  type_ = type;
}

int Gate::vote_number() {
  if (vote_number_ == -1) {
    std::string msg = "Vote number is not set for " +
                      this->orig_id() + " gate.";
    throw ValueError(msg);
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
    throw ValueError(msg);
  } else if (vnumber < 2) {
    std::string msg = "Vote number cannot be less than 2.";
    throw ValueError(msg);
  } else if (vote_number_ != -1) {
    std::string msg = "Trying to re-assign a vote number for " +
                      this->orig_id() + " gate.";
    throw ValueError(msg);
  }  // Children number should be checked outside of this class.
  vote_number_ = vnumber;
}

void Gate::AddChild(const boost::shared_ptr<Event>& child) {
  if (children_.count(child->id())) {
    std::string msg = "Trying to re-insert a child for " + this->orig_id() +
                      " gate.";
    throw ValueError(msg);
  }
  children_.insert(std::make_pair(child->id(), child));
}

void Gate::MergeGate(const boost::shared_ptr<Gate>& gate) {
  assert(gate->type_ != "");
  assert(gate->type_ == type_);

  if (children_.count(gate->id())) children_.erase(gate->id());

  assert(!gate->children_.empty());
  children_.insert(gate->children_.begin(), gate->children_.end());
}

const std::map<std::string, boost::shared_ptr<Event> >& Gate::children() {
  if (children_.empty()) {
    std::string msg = this->orig_id() + " gate does not have children.";
    throw ValueError(msg);
  }
  return children_;
}

}  // namespace scram
