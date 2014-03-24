// Implementation of top event class
#include "event.h"

#include <string>

namespace scram {

TopEvent::TopEvent(std::string id, std::string gate)
    : scram::Event(id),
      gate_(gate) {}

std::string TopEvent::gate() {
  return gate_;
}

void TopEvent::gate(std::string gate) {
  gate_ = gate;
}

const std::map<std::string, scram::Event*>& TopEvent::children() {
  return children_;
}

void TopEvent::add_child(scram::Event* child) {
  children_.insert(std::make_pair(child->id(), child));
}

}  // namespace scram
