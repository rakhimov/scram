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
        << errinfo_element(name_, "element") << errinfo_attribute(attr.name());
  }
}

void Element::SetAttribute(Attribute attr) noexcept {
  auto [it, success] = attributes_.insert(std::move(attr));
  if (!success)
    *it = std::move(attr);
}

const Attribute* Element::GetAttribute(std::string_view name) const noexcept {
  auto it = attributes_.find(name);
  if (it != attributes_.end())
    return &*it;

  return container() ? container()->GetAttribute(name) : nullptr;
}

std::optional<Attribute>
Element::RemoveAttribute(std::string_view name) noexcept {
  std::optional<Attribute> attr;
  auto it = attributes_.find(name);
  if (it != attributes_.end()) {
    attr = std::move(*it);
    attributes_.erase(it);
  }
  return attr;
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
      full_path_(GetFullPath(this)) {}

void Id::id(std::string name) {
  Element::name(std::move(name));
  full_path_ = GetFullPath(this);
}

}  // namespace scram::mef
