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

/// @file element.h
/// Helper classes, structs, and properties
/// common to all other classes.

#ifndef SCRAM_SRC_ELEMENT_H_
#define SCRAM_SRC_ELEMENT_H_

#include <map>
#include <string>

namespace scram {
namespace mef {

/// @struct Attribute
/// This struct allows any attribute.
struct Attribute {
  std::string name;  ///< The name that identifies this attribute.
  std::string value;  ///< Value of this attribute.
  std::string type;  ///< Optional type of the attribute.
};

/// @class Element
/// Mixin class that represents
/// any element of analysis
/// that can have extra descriptions,
/// such as attributes and a label.
class Element {
 public:
  /// Constructs an element with an original name.
  ///
  /// @param[in] name  The local identifier name.
  /// @param[in] optional_name  Allow empty names.
  ///
  /// @throws LogicError  The name is required and empty.
  explicit Element(std::string name, bool optional_name = false);

  /// @returns The original name.
  const std::string& name() const { return kName_; }

  /// @returns The empty or preset label.
  /// @returns Empty string if the label has not been set.
  const std::string& label() const { return label_; }

  /// Sets the label.
  ///
  /// @param[in] new_label  The label to be set.
  ///
  /// @throws LogicError  The label is already set,
  ///                     or the new label is empty.
  void label(const std::string& new_label);

  /// Adds an attribute to the attribute map.
  ///
  /// @param[in] attr  Unique attribute of this element.
  ///
  /// @throws LogicError  The attribute already exists.
  void AddAttribute(const Attribute& attr);

  /// Checks if the element has a given attribute.
  ///
  /// @param[in] id  The identification name of the attribute.
  ///
  /// @returns true if this element has an attribute with the given ID.
  /// @returns false otherwise.
  bool HasAttribute(const std::string& id) const;

  /// @returns Reference to the attribute if it exists.
  ///
  /// @param[in] id  The id name of the attribute.
  ///
  /// @throws LogicError  There is no such attribute.
  const Attribute& GetAttribute(const std::string& id) const;

 protected:
  ~Element() = default;

 private:
  const std::string kName_;  ///< The original name of the element.
  std::string label_;  ///< The label text for the element.
  std::map<std::string, Attribute> attributes_;  ///< Collection of attributes.
};

/// Role, access attributes for elements.
enum class RoleSpecifier { kPublic, kPrivate };

/// @class Role
/// Mixin class that manages private or public roles
/// for elements as needed.
/// Public is the default assumption.
/// It is expected to be set only once and never change.
class Role {
 public:
  /// Sets the role of an element upon creation.
  ///
  /// @param[in] role  A role specifier of the element.
  /// @param[in] base_path  The series of containers to get this event.
  explicit Role(RoleSpecifier role = RoleSpecifier::kPublic,
                std::string base_path = "");

  /// @returns The assigned role of the element.
  RoleSpecifier role() const { return kRole_; }

  /// @returns The base path containing ancestor container names.
  const std::string& base_path() const { return kBasePath_; }

 protected:
  ~Role() = default;

 private:
  const RoleSpecifier kRole_;  ///< The role of the element.
  const std::string kBasePath_;  ///< A series of ancestor containers.
};

/// @class Id
/// Mixin class for assigning unique identifiers to elements.
class Id {
 public:
  /// Mangles the element name to be unique.
  ///
  /// @param[in] el  The owner of the id.
  /// @param[in] role  The role of the element.
  ///
  /// @throws LogicError if name mangling strings are empty.
  Id(const Element& el, const Role& role);

  /// @returns The unique id that is set upon the construction of this element.
  const std::string& id() const { return kId_; }

 protected:
  ~Id() = default;

 private:
  const std::string kId_;  ///< Unique Id name of an element.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_ELEMENT_H_
