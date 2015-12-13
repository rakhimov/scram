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

/// @file element.h
/// Helper classes, structs, and properties
/// common to all other classes.

#ifndef SCRAM_SRC_ELEMENT_H_
#define SCRAM_SRC_ELEMENT_H_

#include <map>
#include <string>

namespace scram {

/// @struct Attribute
/// This struct allows for any attribute.
struct Attribute {
  std::string name;  ///< The name that identifies this attribute.
  std::string value;  ///< Value of this attributes.
  std::string type;  ///< Optional type of the attribute.
};

/// @class Element
/// Mixin class that represents
/// any element of analysis
/// that can have extra descriptions,
/// such as attributes and a label.
class Element {
 public:
  Element();

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
  /// @param[in] id  The identification name of the attribute in lower case.
  ///
  /// @returns true if this element has an attribute with the given ID.
  /// @returns false otherwise.
  bool HasAttribute(const std::string& id) const;

  /// @returns Reference to the attribute if it exists.
  ///
  /// @param[in] id  The id name of the attribute in lower case.
  ///
  /// @throws LogicError  There is no such attribute.
  const Attribute& GetAttribute(const std::string& id) const;

 private:
  std::string label_;  ///< The label for the element.
  std::map<std::string, Attribute> attributes_;  ///< Collection of attributes.
};

/// @class Role
/// Mixin class that manages private or public roles
/// for elements as needed.
/// Public is the default assumption.
/// It is expected to be set only once and never change.
class Role {
 public:
  /// Sets the role of an element upon creation.
  ///
  /// @param[in] is_public  A flag to define public or private role.
  /// @param[in] base_path  The series of containers to get this event.
  explicit Role(bool is_public = true, const std::string& base_path = "");

  /// @returns True for public roles, or False for private roles.
  bool is_public() const { return is_public_; }

  /// @returns The base path containing ancestor container names.
  const std::string& base_path() const { return base_path_; }

 private:
  bool is_public_;  ///< A flag for public and private roles.
  std::string base_path_;  ///< A series of containers leading to this event.
};

}  // namespace scram

#endif  // SCRAM_SRC_ELEMENT_H_
