// Implementation of basic event class
#include "event.h"

#include <string>

#include "error.h"

namespace scram {

BasicEvent::BasicEvent(std::string id, double p)
    : scram::Event(id),
    p_(p) {}

double BasicEvent::p() throw(scram::ValueError) {
  if (p_ == -1) {
    std::string msg = "Probability has not been set.";
    throw scram::ValueError(msg);
  }
  return p_;
}

void BasicEvent::p(double p) throw(scram::ValueError) {
  if (p < 0 || p > 1) {
    std::string msg = "The value for probability is not valid.";
    throw scram::ValueError(msg);
  }

  if (p_ != -1) {
    std::string msg = "Trying to re-assign probability for this event.";
    throw scram::ValueError(msg);
  }

  p_ = p;
}

void BasicEvent::add_parent(scram::Event* parent) {
  parents_.insert(std::make_pair(parent->id(), parent));
}

std::map<std::string, scram::Event*>& BasicEvent::parents() {
  return parents_;
}

}  // namespace scram
