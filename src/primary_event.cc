// Implementation of basic event class
#include "event.h"

#include <string>

#include "error.h"

namespace scram {

PrimaryEvent::PrimaryEvent(std::string id, std::string type, double p)
    : scram::Event(id),
      type_(type),
      p_(p) {
}

std::string PrimaryEvent::type() {
  if (type_ == "") {
    std::string msg = "Type has not been set.";
    throw scram::ValueError(msg);
  }
  return type_;
}

void PrimaryEvent::type(std::string new_type) {
  if (type_ != "") {
    std::string msg = "Trying to re-assign the type.";
    throw scram::ValueError(msg);
  }

  type_ = new_type;
}

double PrimaryEvent::p() {
  if (p_ == -1) {
    std::string msg = "Probability has not been set.";
    throw scram::ValueError(msg);
  }
  return p_;
}

void PrimaryEvent::p(double p) {
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

void PrimaryEvent::add_parent(scram::Event* parent) {
  if (parents_.count(parent->id())) {
    std::string msg = "Trying to re-insert existing parent.";
    throw scram::ValueError(msg);
  }

  parents_.insert(std::make_pair(parent->id(), parent));
}

std::map<std::string, scram::Event*>& PrimaryEvent::parents() {
  if (parents_.empty()) {
    std::string msg = "This basic event does not have parents.";
    throw scram::ValueError(msg);
  }

  return parents_;
}

}  // namespace scram
