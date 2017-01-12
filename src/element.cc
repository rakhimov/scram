/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#include <boost/range/algorithm.hpp>

#include "error.h"
#include "ext/algorithm.h"

namespace scram {
namespace mef {

Element::Element(std::string name, bool optional_name)
    : kName_(std::move(name)) {
  if (!optional_name && kName_.empty())
    throw LogicError("The element name can't be empty");
  if (!kName_.empty() && kName_.find('.') != std::string::npos)
    throw InvalidArgument("The element name is malformed.");
}

void Element::label(std::string new_label) {
  if (!label_.empty())
    throw LogicError("Trying to reset the label: " + label_);
  if (new_label.empty())
    throw LogicError("Trying to apply empty label");

  label_ = std::move(new_label);
}

void Element::AddAttribute(Attribute attr) {
  if (HasAttribute(attr.name)) {
    throw DuplicateArgumentError(
        "Trying to overwrite an existing attribute {event: " + kName_ +
        ", attr: " + attr.name + "} ");
  }
  attributes_.emplace_back(std::move(attr));
}

bool Element::HasAttribute(const std::string& name) const {
  return ext::any_of(attributes_, [&name](const Attribute& attr) {
    return attr.name == name;
  });
}

const Attribute& Element::GetAttribute(const std::string& name) const {
  auto it = boost::find_if(attributes_, [&name](const Attribute& attr) {
    return attr.name == name;
  });
  if (it == attributes_.end())
    throw LogicError("Element does not have attribute: " + name);

  return *it;
}

Role::Role(RoleSpecifier role, std::string base_path)
    : kBasePath_(std::move(base_path)),
      kRole_(role) {
  if (!kBasePath_.empty() &&
      (kBasePath_.front() == '.' || kBasePath_.back() == '.')) {
    throw InvalidArgument("Element reference base path is malformed.");
  }
}

Id::Id(const Element& el, const Role& role)
    : kId_(role.role() == RoleSpecifier::kPublic
               ? el.name()
               : role.base_path() + "." + el.name()) {
  if (el.name().empty())
    throw LogicError("The name for an Id is empty!");
  if (role.role() == RoleSpecifier::kPrivate && role.base_path().empty())
    throw LogicError("The base path for a private element is empty.");
}

}  // namespace mef
}  // namespace scram
