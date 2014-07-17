// Implementation of primary event class.
#include "event.h"

#include <math.h>

#include "error.h"

namespace scram {

PrimaryEvent::PrimaryEvent(std::string id, std::string type, double p)
    : scram::Event(id),
      type_(type),
      p_(p) {}

std::string PrimaryEvent::type() {
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

  if (type_ == "house") {
    if (p != 0 && p != 1) {
      std::string msg = "Incorrect probability for house event: " + this->id();
      throw scram::ValueError(msg);
    }
  }
  p_ = p;
}

void PrimaryEvent::p(double p, double time) {
  if (p_ != -1) {
    std::string msg = "Trying to re-assign probability for " + this->id();
    throw scram::ValueError(msg);
  }

  if (p < 0) {
    std::string msg = "The value for a failure rate is not valid for " +
        this->id();
    throw scram::ValueError(msg);
  }

  if (time < 0) {
    std::string msg = "The value for time is not valid for " +
        this->id();
    throw scram::ValueError(msg);
  }

  if (type_ == "house") {
    std::string msg = "House event can't be defined by a failure rate: " +
        this->id();
    throw scram::ValueError(msg);
  }

  p_ = 1 - exp(p * time);
}

void PrimaryEvent::AddParent(const boost::shared_ptr<scram::TopEvent>& parent) {
  if (parents_.count(parent->id())) {
    std::string msg = "Trying to re-insert existing parent for " + this->id();
    throw scram::ValueError(msg);
  }
  parents_.insert(std::make_pair(parent->id(), parent));
}

std::map<std::string,
    boost::shared_ptr<scram::TopEvent> >& PrimaryEvent::parents() {
  if (parents_.empty()) {
    std::string msg = this->id() + " primary event does not have parents.";
    throw scram::ValueError(msg);
  }
  return parents_;
}

}  // namespace scram
