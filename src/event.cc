/// @file event.cc
/// Implementation of Event Class.
#include "event.h"

namespace scram {

Event::Event(std::string id) : id_(id) {}

void Event::AddParent(const boost::shared_ptr<scram::Gate>& parent) {
  if (parents_.count(parent->id())) {
    std::string msg = "Trying to re-insert existing parent for " + this->id();
    throw scram::ValueError(msg);
  }
  parents_.insert(std::make_pair(parent->id(), parent));
}

const std::map<std::string, boost::shared_ptr<scram::Gate> >& Event::parents() {
  if (parents_.empty()) {
    std::string msg = this->id() + " does not have parents.";
    throw scram::ValueError(msg);
  }
  return parents_;
}

}  // namespace scram
