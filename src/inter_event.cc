// Implementation of intermidiate event class.
#include "event.h"

#include <string>

#include "error.h"

namespace scram {

InterEvent::InterEvent(std::string id, std::string gate)
    : scram::TopEvent(id, gate) {}

boost::shared_ptr<scram::Event> InterEvent::parent() {
  if (!parent_) {
    std::string msg = "Parent is not set for " + this->id();
    throw scram::ValueError(msg);
  }

  return parent_;
}

void InterEvent::parent(boost::shared_ptr<scram::Event> parent) {
  if (parent_) {
    std::string msg = "Trying to re-assign a parent for " + this->id();
    throw scram::ValueError(msg);
  }

  parent_ = parent;
}

}  // namespace scram
