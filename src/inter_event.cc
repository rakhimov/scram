/// @file inter_event.cc
/// Implementation of intermidiate event class.
#include "event.h"

#include "error.h"

namespace scram {

InterEvent::InterEvent(std::string id, std::string gate)
    : scram::TopEvent(id, gate) {}

const boost::shared_ptr<scram::TopEvent>& InterEvent::parent() {
  if (!parent_) {
    std::string msg = "Parent is not set for " + this->id();
    throw scram::ValueError(msg);
  }
  return parent_;
}

void InterEvent::parent(const boost::shared_ptr<scram::TopEvent>& parent) {
  if (parent_) {
    std::string msg = "Trying to re-assign a parent for " + this->id();
    throw scram::ValueError(msg);
  }
  parent_ = parent;
}

}  // namespace scram
