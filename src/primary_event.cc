/// @file primary_event.cc
/// Implementation of primary, base, house event classes.
#include "event.h"

#include "error.h"

namespace scram {

PrimaryEvent::PrimaryEvent(std::string id, std::string type)
    : scram::Event(id),
      type_(type) {}

BasicEvent::BasicEvent(std::string id) : scram::PrimaryEvent(id, "basic") {}

HouseEvent::HouseEvent(std::string id)
    : scram::PrimaryEvent(id, "house"),
      state_(false) {}

}  // namespace scram
