/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of helper features for all elements of the analysis.

#include "element.h"

namespace scram::mef {

Element::Element(std::string name) { Element::name(std::move(name)); }

void Element::name(std::string name) {
  if (name.empty())
    SCRAM_THROW(LogicError("The element name cannot be empty"));
  if (name.find('.') != std::string::npos)
    SCRAM_THROW(ValidityError("The element name is malformed."));
  name_ = std::move(name);
}

void Element::AddAttribute(Attribute attr) {
  if (attributes_.insert(std::move(attr)).second == false) {
    SCRAM_THROW(ValidityError("Duplicate attribute"))
        << errinfo_element(name_, "element") << errinfo_attribute(attr.name);
  }
}

void Element::SetAttribute(Attribute attr) {
  auto [it, success] = attributes_.insert(std::move(attr));
  if (!success) {
    it->value = std::move(attr.value);
    it->type = std::move(attr.type);
  }
}

const Attribute& Element::GetAttribute(const std::string& name) const {
  auto it = attributes_.find(name);
  if (it == attributes_.end())
    SCRAM_THROW(LogicError("Element does not have attribute: " + name));

  return *it;
}

Role::Role(RoleSpecifier role, std::string base_path)
    : kBasePath_(std::move(base_path)), kRole_(role) {
  if (!kBasePath_.empty() &&
      (kBasePath_.front() == '.' || kBasePath_.back() == '.')) {
    SCRAM_THROW(ValidityError("Element reference base path is malformed."));
  }
  if (kRole_ == RoleSpecifier::kPrivate && kBasePath_.empty())
    SCRAM_THROW(ValidityError("Elements cannot be private at model scope."));
}

Id::Id(std::string name, std::string base_path, RoleSpecifier role)
    : Element(std::move(name)),
      Role(role, std::move(base_path)),
      id_(MakeId(*this)) {}

void Id::id(std::string name) {
  Element::name(std::move(name));
  id_ = MakeId(*this);
}

}  // namespace scram::mef
