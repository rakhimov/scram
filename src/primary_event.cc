/// @file primary_event.cc
/// Implementation of primary, base, house event classes.
#include "event.h"

#include "error.h"

namespace scram {

PrimaryEvent::PrimaryEvent(std::string id, std::string type)
    : scram::Event(id),
      type_(type),
      p_(-1) {}

BasicEvent::BasicEvent(std::string id) : scram::PrimaryEvent(id, "basic") {}

HouseEvent::HouseEvent(std::string id)
    : scram::PrimaryEvent(id, "house"),
      state_(false) {}

const std::string& PrimaryEvent::type() {
  if (type_ == "") {
    std::string msg = this->id() + " type has not been set.";
    throw scram::ValueError(msg);
  }
  return type_;
}

void PrimaryEvent::type(std::string new_type) {
  if (type_ != "") {
    std::string msg = "Trying to re-assign the type of " + this->id();
    throw scram::ValueError(msg);
  }
  type_ = new_type;
}

double PrimaryEvent::p() {
  if (p_ == -1) {
    std::string msg = "Probability has not been set for " + this->id();
    throw scram::ValueError(msg);
  }
  return p_;
}

void PrimaryEvent::p(double p) {
  if (p_ != -1) {
    std::string msg = "Trying to re-assign probability for " + this->id();
    throw scram::ValueError(msg);
  }
  if (p < 0 || p > 1) {
    std::string msg = "The value for probability is not valid for " +
        this->id();
    throw scram::ValueError(msg);
  }
  p_ = p;
}

void HouseEvent::p(double p) {
  if (p != 0 && p != 1) {
    std::string msg = "Incorrect probability for house event: " + this->id();
    throw scram::ValueError(msg);
  }
  state_ = (p == 1) ? true : false;
  PrimaryEvent::p(p);
}

}  // namespace scram
