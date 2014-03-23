// Implementation of basic event class
#include "event.h"

#include <string>

namespace scram {

BasicEvent::BasicEvent(std::string id, double p)
    : scram::Event(id),
      p_(p) {}

double BasicEvent::p() {
  return p_;
}

void BasicEvent::p(double p) {
  p_ = p;
}

void BasicEvent::add_parent(scram::Event* parent) {
  parents_.insert(std::make_pair(parent->id(), parent));
}

std::map<std::string, scram::Event*>& BasicEvent::parents() {
  return parents_;
}

}  // namespace scram
