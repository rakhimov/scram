// Implementation of intermidiate event class
#include "event.h"

#include <string>

namespace scram {

InterEvent::InterEvent(std::string id, std::string gate,
                       scram::Event* parent)
    : scram::TopEvent(id, gate),
      parent_(parent) {}

const scram::Event* InterEvent::parent() {
  return parent_;
}

void InterEvent::parent(scram::Event* parent) {
  parent_ = parent;
}

}  // namespace scram
