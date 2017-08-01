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

/// @file element.h
/// Helper classes, structs, and properties
/// common to all other classes.

#ifndef SCRAM_SRC_ELEMENT_H_
#define SCRAM_SRC_ELEMENT_H_

#include <cstdint>

#include <string>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace scram {
namespace mef {

/// This struct allows any attribute.
struct Attribute {
  std::string name;  ///< The name that identifies this attribute.
  std::string value;  ///< Value of this attribute.
  std::string type;  ///< Optional type of the attribute.
};

/// Mixin class that represents
/// any element of analysis
/// that can have extra descriptions,
/// such as attributes and a label.
class Element {
 public:
  /// Constructs an element with an original name.
  /// The name is expected to conform to identifier requirements
  /// described in the MEF documentation and additions.
  ///
  /// @param[in] name  The local identifier name.
  ///
  /// @throws LogicError  The name is required and empty.
  /// @throws InvalidArgument  The name is malformed.
  explicit Element(std::string name);

  /// @returns The original name.
  const std::string& name() const { return name_; }

  /// @returns The empty or preset label.
  /// @returns Empty string if the label has not been set.
  const std::string& label() const { return label_; }

  /// Sets the label.
  ///
  /// @param[in] label  The label text to be set.
  void label(std::string label) { label_ = std::move(label); }

  /// @returns The current set of element attributes.
  const std::vector<Attribute>& attributes() const { return attributes_; }

  /// Adds an attribute to the attribute map.
  ///
  /// @param[in] attr  Unique attribute of this element.
  ///
  /// @throws DuplicateArgumentError  A member attribute with the same name
  ///                                 already exists.
  ///
  /// @post Pointers or references
  ///       to existing attributes may get invalidated.
  void AddAttribute(Attribute attr);

  /// Sets an attribute to the attribute map.
  /// If an attribute with the same name exits,
  /// it gets overwritten.
  ///
  /// @param[in] attr  An attribute of this element.
  ///
  /// @post Pointers or references
  ///       to existing attributes may get invalidated.
  void SetAttribute(Attribute attr);

  /// Checks if the element has a given attribute.
  ///
  /// @param[in] name  The identifying name of the attribute.
  ///
  /// @returns true if this element has an attribute with the given name.
  bool HasAttribute(const std::string& name) const;

  /// @returns A member attribute with the given name.
  ///
  /// @param[in] name  The id name of the attribute.
  ///
  /// @throws LogicError  There is no such attribute.
  const Attribute& GetAttribute(const std::string& name) const;

  /// Removes the attribute of the element.
  ///
  /// @param[in] name  The identifying name of the attribute.
  ///
  /// @returns false No such attribute to remove.
  bool RemoveAttribute(const std::string& name);

 protected:
  ~Element() = default;

  /// Resets the element name.
  ///
  /// @param[in] name  The local identifier name.
  ///
  /// @throws LogicError  The name is required and empty.
  /// @throws InvalidArgument  The name is malformed.
  void name(std::string name);

 private:
  std::string name_;  ///< The original name of the element.
  std::string label_;  ///< The label text for the element.

  /// Container of attributes ordered by insertion time.
  /// The attributes are unique by their names.
  ///
  /// @note Using a hash table incurs a huge memory overhead (~400B / element).
  /// @note Elements are expected to have few attributes,
  ///       complex containers may be overkill.
  std::vector<Attribute> attributes_;
};

/// Table of elements with unique names.
///
/// @tparam T  Value or (smart/raw) pointer type deriving from Element class.
///
/// @pre The element names are not modified
///      while it is in the container.
template <typename T>
using ElementTable = boost::multi_index_container<
    T, boost::multi_index::indexed_by<
           boost::multi_index::hashed_unique<boost::multi_index::const_mem_fun<
               Element, const std::string&, &Element::name>>>>;

/// Role, access attributes for elements.
enum class RoleSpecifier : std::uint8_t { kPublic, kPrivate };

/// Mixin class that manages private or public roles
/// for elements as needed.
/// Public is the default assumption.
/// It is expected to be set only once and never change.
class Role {
 public:
  /// Sets the role of an element upon creation.
  /// The base reference path must be formatted
  /// according to the MEF documentation and additions.
  ///
  /// @param[in] role  A role specifier of the element.
  /// @param[in] base_path  The series of containers to get this event.
  ///
  /// @throws InvalidArgument  The base path string is malformed.
  /// @throws ValidationError  Private element at model/global scope.
  explicit Role(RoleSpecifier role = RoleSpecifier::kPublic,
                std::string base_path = "");

  /// @returns The assigned role of the element.
  RoleSpecifier role() const { return kRole_; }

  /// @returns The base path containing ancestor container names.
  const std::string& base_path() const { return kBasePath_; }

 protected:
  ~Role() = default;

 private:
  const std::string kBasePath_;  ///< A series of ancestor containers.
  const RoleSpecifier kRole_;  ///< The role of the element.
};

/// Computes the full path of an element.
///
/// @tparam T  Element type deriving from Role.
///
/// @param[in] element  A valid element with a name and base path.
///
/// @returns A string representation of the full path.
template <typename T>
std::string GetFullPath(const T* element) {
  return element->base_path() + "." + element->name();
}

/// Mixin class for assigning unique identifiers to elements.
class Id : public Element, public Role {
 public:
  /// @copydoc Element::Element
  /// @copydoc Role::Role
  ///
  /// Mangles the element name into a unique id.
  /// Private elements get their full path as their ids,
  /// while public elements retain their name as ids.
  explicit Id(std::string name, std::string base_path = "",
              RoleSpecifier role = RoleSpecifier::kPublic);

  /// @returns The unique id that is set upon the construction of this element.
  const std::string& id() const { return id_; }

  /// Resets the element ID.
  ///
  /// @param[in] name  The new valid name for the element.
  ///
  /// @pre The element is not in any container keyed by its ID or name.
  ///
  /// @throws LogicError  The name is empty.
  /// @throws InvalidArgument  The name is malformed.
  void id(std::string name);

  /// Produces unique name for the model element within the same type.
  /// @{
  static const std::string& unique_name(const Element& element) {
    return element.name();
  }
  static const std::string& unique_name(const Id& element) {
    return element.id();
  }
  /// @}

 protected:
  ~Id() = default;

 private:
  /// Creates an ID string for an element.
  static std::string MakeId(const Id& element) {
    return element.role() == RoleSpecifier::kPublic ? element.name()
                                                    : GetFullPath(&element);
  }

  std::string id_;  ///< Unique Id name of an element.
};

/// Table of elements with unique ids.
///
/// @tparam T  Value or (smart/raw) pointer type deriving from Id class.
///
/// @pre The element IDs are not modified
///      while it is in the container.
template <typename T>
using IdTable = boost::multi_index_container<
    T,
    boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
        boost::multi_index::const_mem_fun<Id, const std::string&, &Id::id>>>>;

/// Adds a unique element into a table,
/// ensuring no duplicated entries.
///
/// @tparam ErrorType  The error type to indicate the insertion of a duplicate.
/// @tparam T  A pointer type of the element supporting Element API.
/// @tparam Table  A set container supporting standard insert API.
///
/// @param[in] element  The pointer to the unique element.
/// @param[in,out] table  The destination set container.
/// @param[in] header  The error message header appearing before duplicate name.
///
/// @throws ErrorType  The element is already in the table.
template <class ErrorType, class T, class Table>
void AddElement(T&& element, Table* table, const char* header) {
  const std::string& name = element->name();  // The element pointer may move!
  if (table->insert(std::forward<T>(element)).second == false) {
    throw ErrorType(header + name);
  }
}

/// Mixin class for providing marks for graph nodes.
class NodeMark {
 public:
  /// Possible marks for the node.
  enum Mark : std::uint8_t {
    kClear = 0,  ///< Implicit conversion to Boolean false.
    kTemporary,
    kPermanent
  };

  /// @returns The mark of this node.
  Mark mark() const { return mark_; }

  /// Sets the mark for this node.
  ///
  /// @param[in] label  The specific label for the node.
  void mark(Mark label) { mark_ = label; }

 protected:
  ~NodeMark() = default;

 private:
  Mark mark_ = kClear;  ///< The mark for traversal or toposort.
};

/// Mixin class for providing usage marks for elements.
class Usage {
 public:
  /// @returns true if the element is used in the model or analysis.
  bool usage() const { return usage_; }

  /// @param[in] usage  The usage state of the element in a model.
  void usage(bool usage) { usage_ = usage; }

 protected:
  ~Usage() = default;

 private:
  bool usage_ = false;  ///< Elements are assumed to be unused at construction.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_ELEMENT_H_
