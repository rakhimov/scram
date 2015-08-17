/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

Role::Role(bool is_public, const std::string& base_path)
      : is_public_(is_public),
        base_path_(base_path) {}

Role::~Role() {}  // Empty body for pure virtual destructor.

}  // namespace scram
