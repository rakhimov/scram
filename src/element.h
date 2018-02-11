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
/// Helper classes, structs, and properties
/// common to all other classes.

#pragma once

#include <cstdint>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/noncopyable.hpp>

#include "error.h"
#include "ext/linear_set.h"
#include "ext/multi_index.h"

namespace scram::mef {

class Element;

/// Elements in MEF containers.
class ContainerElement {
  template <class, class, bool, bool>
  friend class Container;  // Only containers manage their elements.

 protected:
  /// @returns The pointer to the parent container.
  ///          nullptr if no container is provided.
  const Element* container() const { return container_; }

 private:
  /// @param[in] element  The element that is the parent container.
  ///                     nullptr if no container.
  void container(const Element* element) { container_ = element; }

  /// The optional reference to the parent container element
  /// that this element is a member of.
  ///
  /// @note Attributes are inherited from the parent container.
  const Element* container_ = nullptr;
};

/// MEF Element Attributes.
/// Attributes carry extra (arbitrary) data for the elements.
/// The interpretation of the attribute data
/// is tool-dependent or up-to-the-user.
///
/// @note Attribute values are all free-form strings.
///       The strings are not sanitized or normalized to be meaningful.
///       If not careful, it is possible to end-up with values
///       that XML schema validators won't accept (e.g., special chars).
class Attribute {
 public:
  /// @param[in] name  The name for the attribute.
  /// @param[in] value  The value for the attribute.
  /// @param[in] type  The optional type of the attribute value.
  ///
  /// @throws LogicError  if any required values are empty.
  Attribute(std::string name, std::string value, std::string type = "") {
    Attribute::name(std::move(name));
    Attribute::value(std::move(value));
    Attribute::type(std::move(type));
  }

  /// @returns The name of the attribute.
  const std::string& name() const { return name_; }

  /// @param[in] name  The name for the attribute.
  ///
  /// @throws LogicError  The name is empty.
  void name(std::string name) {
    if (name.empty())
      SCRAM_THROW(LogicError("Attribute name cannot be empty."));
    name_ = std::move(name);
  }

  /// @returns The value of the attribute.
  const std::string& value() const { return value_; }

  /// @param[in] value  The value for the attribute.
  ///
  /// @throws LogicError  The value is empty.
  void value(std::string value) {
    if (value.empty())
      SCRAM_THROW(LogicError("Attribute value cannot be empty."));
    value_ = std::move(value);
  }

  /// @returns The type of the attribute value.
  ///          Empty string if the type is not set.
  const std::string& type() const { return type_; }

  /// @param[in] type  The value type.
  ///                  Empty string to remove the type.
  void type(std::string type) { type_ = std::move(type); }

 private:
  std::string name_;  ///< The name that identifies this attribute.
  std::string value_;  ///< Value of this attribute.
  std::string type_;  ///< Optional type of the attribute.
};

/// Mixin class that represents
/// any element of analysis
/// that can have extra descriptions,
/// such as attributes and a label.
class Element : public ContainerElement, private boost::noncopyable {
 public:
  /// Constructs an element with an original name.
  /// The name is expected to conform to identifier requirements
  /// described in the MEF documentation and additions.
  ///
  /// @param[in] name  The local identifier name.
  ///
  /// @throws LogicError  The name is required and empty.
  /// @throws ValidityError  The name is malformed.
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
  const std::vector<Attribute>& attributes() const {
    return attributes_.data();
  }

  /// Adds an attribute to the attribute map.
  ///
  /// @param[in] attr  Unique attribute of this element.
  ///
  /// @throws ValidityError  A member attribute with the same name
  ///                        already exists.
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
  bool HasAttribute(const std::string& name) const {
    return attributes_.count(name);
  }

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
  bool RemoveAttribute(const std::string& name) {
    return attributes_.erase(name);
  }

 protected:
  ~Element() = default;

  /// Resets the element name.
  ///
  /// @param[in] name  The local identifier name.
  ///
  /// @throws LogicError  The name is required and empty.
  /// @throws ValidityError  The name is malformed.
  void name(std::string name);

 private:
  std::string name_;  ///< The original name of the element.
  std::string label_;  ///< The label text for the element.

  /// Attribute equality predicate for unique entries.
  struct AttributeKey {
    /// Tests attribute equality with attribute names.
    const std::string& operator()(const Attribute& attribute) const {
      return attribute.name();
    }
  };

  /// Element attributes ordered by insertion time.
  /// The attributes are unique by their names.
  ///
  /// @note The element attributes override its inherited attributes.
  ///
  /// @note Using a hash table incurs a huge memory overhead (~400B / element).
  /// @note Elements are expected to have very few attributes,
  ///       complex containers may be overkill.
  ext::linear_set<Attribute, AttributeKey> attributes_;
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
  /// @throws ValidityError  The base path string is malformed.
  /// @throws ValidityError  Private element at model/global scope.
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
  /// @throws ValidityError  The name is malformed.
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

/// The MEF Container of unique elements.
///
/// @tparam Self  The deriving MEF container type for the CRTP.
/// @tparam T  The MEF element type stored in the container.
/// @tparam Ownership  True if the container takes ownership over the elements.
/// @tparam ById  The indexation strategy (ID or name) to keep elements unique.
template <class Self, class T, bool Ownership = true,
          bool ById = std::is_base_of_v<Id, T>>
class Container {
 public:
  /// The MEF Element type.
  using ElementType = T;
  /// The pointer type (owning or not) to store in the table.
  using Pointer = std::conditional_t<Ownership, std::unique_ptr<T>, T*>;
  /// The table indexed by id or name.
  using TableType =
      std::conditional_t<ById, IdTable<Pointer>, ElementTable<Pointer>>;

  /// @returns The underlying table with the elements.
  const TableType& table() const { return table_; }

  /// Adds a unique element into the container,
  /// ensuring no duplicated entries.
  ///
  /// @param[in] element  The pointer to the unique element.
  ///
  /// @throws DuplicateElementError  The element is already in the container.
  void Add(Pointer element) {
    T& stable_ref = *element;  // The pointer will be moved later.
    if (table_.insert(std::move(element)).second == false) {
      SCRAM_THROW(DuplicateElementError())
          << errinfo_element(Id::unique_name(stable_ref), T::kTypeString)
          << errinfo_container(Id::unique_name(static_cast<Self&>(*this)),
                               Self::kTypeString);
    }
    stable_ref.container(static_cast<const Self*>(this));
  }

  /// Removes MEF elements from the container.
  ///
  /// @param[in] element  An element defined in this container.
  ///
  /// @returns The removed element.
  ///
  /// @throws UndefinedElement  The element cannot be found in the container.
  /// @throws LogicError  The element in the container is not the same object.
  Pointer Remove(T* element) {
    const std::string& key = [element]() -> decltype(auto) {
      if constexpr (ById) {
        return element->id();
      } else {
        return element->name();
      }
    }();

    auto it = table_.find(key);

    try {
      if (it == table_.end())
        SCRAM_THROW(UndefinedElement("Undefined Element"));
      if (&**it != element)
        SCRAM_THROW(LogicError("Duplicate element with different address."));

    } catch (Error& err) {
      err << errinfo_element(Id::unique_name(*element), T::kTypeString)
          << errinfo_container(Id::unique_name(static_cast<Self&>(*this)),
                               Self::kTypeString);
      throw;
    }
    element->container(nullptr);
    return ext::extract(it, &table_);  // no-throw.
  }

 private:
  TableType table_;  ///< Unique table with the elements.
};

namespace detail {  // Composite container helper facilities.

/// Implementation of container_of to deal w/ empty type list.
template <class E, class T, class... Ts>
struct container_of_impl {
  /// The type of the container with elements of type T.
  using type = std::conditional_t<std::is_same_v<E, typename T::ElementType>, T,
                                  typename container_of_impl<E, Ts...>::type>;
};

/// Specialization for an empty type list.
template <class E>
struct container_of_impl<E, void> {
  using type = void;  ///< The indicator of not-found.
};

/// Finds the container type for the given element type.
///
/// @tparam E  The mef::Element type.
/// @tparam T  The mef::Container type.
/// @tparam Ts  Other mef::Container types in the type list.
template <class E, class T, class... Ts>
struct container_of : public container_of_impl<E, T, Ts..., void> {};

}  // namespace detail

/// The composition of multiple mef::Containers.
///
/// @tparam Ts  Container types.
template <typename... Ts>
class Composite : public Ts... {
 public:
  using Ts::Add...;
  using Ts::Remove...;

  /// @tparam T  The mef::Element type in the composite container.
  ///
  /// @returns The table containing the elements of the given type.
  template <class T>
  decltype(auto) table() const {
    using ContainerType = typename detail::container_of<T, Ts...>::type;
    static_assert(!std::is_same_v<ContainerType, void>,
                  "No container with elements of type T.");
    return ContainerType::table();
  }
};

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

}  // namespace scram::mef
