/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

Element::Element(std::string name) { Element::name(std::move(name)); }

void Element::name(std::string name) {
  if (name.empty())
    throw LogicError("The element name cannot be empty");
  if (name.find('.') != std::string::npos)
    throw InvalidArgument("The element name is malformed.");
  name_ = std::move(name);
}

void Element::AddAttribute(Attribute attr) {
  if (HasAttribute(attr.name)) {
    throw DuplicateArgumentError(
        "Trying to overwrite an existing attribute {event: " + name_ +
        ", attr: " + attr.name + "} ");
  }
  attributes_.emplace_back(std::move(attr));
}

void Element::SetAttribute(Attribute attr) {
  auto it = boost::find_if(attributes_, [&attr](const Attribute& member) {
    return attr.name == member.name;
  });
  if (it != attributes_.end()) {
    it->value = std::move(attr.value);
    it->type = std::move(attr.type);
  } else {
    attributes_.emplace_back(std::move(attr));
  }
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

bool Element::RemoveAttribute(const std::string& name) {
  auto it = boost::find_if(attributes_, [&name](const Attribute& attr) {
    return attr.name == name;
  });
  if (it == attributes_.end())
    return false;
  attributes_.erase(it);
  return true;
}

Role::Role(RoleSpecifier role, std::string base_path)
    : kBasePath_(std::move(base_path)),
      kRole_(role) {
  if (!kBasePath_.empty() &&
      (kBasePath_.front() == '.' || kBasePath_.back() == '.')) {
    throw InvalidArgument("Element reference base path is malformed.");
  }
  if (kRole_ == RoleSpecifier::kPrivate && kBasePath_.empty())
    throw ValidationError("Elements cannot be private at model scope.");
}

Id::Id(std::string name, std::string base_path, RoleSpecifier role)
    : Element(std::move(name)),
      Role(role, std::move(base_path)),
      id_(MakeId(*this)) {}

void Id::id(std::string name) {
  Element::name(std::move(name));
  id_ = MakeId(*this);
}

}  // namespace mef
}  // namespace scram
