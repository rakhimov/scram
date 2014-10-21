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

}  // namespace scram
