/// @file element.cc
/// Implementation of helper features for all elements of the analysis.
#include "element.h"

#include <boost/algorithm/string.hpp>

#include "error.h"

namespace scram {

Attribute::Attribute() : name(""), value(""), type("") {}

Element::Element() : label_("") {}

Element::~Element() {}  // Empty body for pure virtual destructor.

void Element::label(const std::string& new_label) {
  if (label_ != "") {
    throw LogicError("Trying to reset the label: " + label_);
  }
  if (new_label == "") {
    throw LogicError("Trying to apply empty label");
  }
  label_ = new_label;
}

void Element::AddAttribute(const Attribute& attr) {
  std::string id = attr.name;
  boost::to_lower(id);
  if (attributes_.count(id)) {
    throw LogicError("Trying to re-add an attribute: " + attr.name);
  }
  attributes_.insert(std::make_pair(id, attr));
}

bool Element::HasAttribute(const std::string& id) const {
  return attributes_.count(id);
}

const Attribute& Element::GetAttribute(const std::string& id) const {
  if (!attributes_.count(id)) {
    throw LogicError("Element does not have attribute: " + id);
  }
  return attributes_.find(id)->second;
}

Role::~Role() {}  // Empty body for pure virtual destructor.

}  // namespace scram
