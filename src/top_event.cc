// Implementation of top event class.
#include "event.h"

#include <string>

#include "error.h"

namespace scram {

TopEvent::TopEvent(std::string id, std::string gate)
    : scram::Event(id),
      gate_(gate) {}

std::string TopEvent::gate() {
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

const std::map<std::string,
      boost::shared_ptr<scram::Event> >& TopEvent::children() {
  if (children_.empty()) {
    std::string msg = this->id() + " event does not have children.";
    throw scram::ValueError(msg);
  }

  return children_;
}

void TopEvent::AddChild(boost::shared_ptr<scram::Event> child) {
  if (children_.count(child->id())) {
    std::string msg = "Trying to re-insert a child for " + this->id() +
                      " event.";
    throw scram::ValueError(msg);
  }

  children_.insert(std::make_pair(child->id(), child));
}

}  // namespace scram
