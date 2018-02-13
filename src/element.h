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
/// Base/mixin classes, structs, and properties
/// common to all MEF classes/constructs.

#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
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

/// The MEF Element
/// with attributes and a label.
/// This is a base/mixin class for most of the MEF constructs.
///
/// @note The class is not polymorphic.
class Element : public ContainerElement, private boost::noncopyable {
  /// Attribute key extractor.
  struct AttributeKey {
    /// Attributes are keyed by their names.
    std::string_view operator()(const Attribute& attribute) const {
      return attribute.name();
    }
  };

 public:
  /// Unique attribute map keyed with the attribute names.
  ///
  /// @note Elements are expected to have very few attributes,
  ///       complex containers may be overkill.
  /// @note Using a multi-index or other tables incurs
  ///       a huge memory overhead in common usage (up to 400B / attribute).
  using AttributeMap = ext::linear_set<Attribute, AttributeKey>;

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

  /// @returns The string view to the name for table keys.
  std::string_view name_view() const { return name_; }

  /// @returns The empty or preset label.
  /// @returns Empty string if the label has not been set.
  const std::string& label() const { return label_; }

  /// Sets the element label.
  ///
  /// @param[in] label  The extra description for the element.
  void label(std::string label) { label_ = std::move(label); }

  /// @returns The current set of element attributes (non-inherited!).
  ///
  /// @note The element attributes override its inherited attributes.
  ///       However, the inherited attributes are not copied into the map.
  ///       The precedence is followed upon lookup.
  const AttributeMap& attributes() const { return attributes_; }

  /// Adds an attribute to the attribute map of this element.
  ///
  /// @param[in] attr  An attribute of this element.
  ///
  /// @throws ValidityError  The attribute is duplicate.
  ///
  /// @warning Pointers or references
  ///          to existing attributes may get invalidated.
  void AddAttribute(Attribute attr);

  /// Sets an attribute to the attribute map.
  /// If an attribute with the same name exits,
  /// it gets overwritten.
  ///
  /// @param[in] attr  An attribute of this element.
  ///
  /// @warning Pointers or references
  ///          to existing attributes may get invalidated.
  void SetAttribute(Attribute attr) noexcept;

  /// @param[in] name  The name of the attribute.
  ///
  /// @returns The attribute with the given name.
  ///          nullptr if no attribute is found.
  ///
  /// @warning Attribute addresses are not stable.
  ///          Do not store the returned pointer.
  ///
  /// @note Attributes can be inherited from parent containers.
  const Attribute* GetAttribute(std::string_view name) const noexcept;

  /// Removes an attribute of this element.
  ///
  /// @param[in] name  The identifying name of the attribute.
  ///
  /// @returns The removed attribute if any.
  ///
  /// @post No inherited attributes are affected.
  std::optional<Attribute> RemoveAttribute(std::string_view name) noexcept;

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

  /// Element attributes ordered by insertion time.
  /// The attributes are unique by their names.
  AttributeMap attributes_;
};

/// Table of elements with unique names.
///
/// @tparam T  Value or (smart/raw) pointer type deriving from Element class.
///
/// @pre The element names are not modified
///      while it is in the container.
template <typename T>
using ElementTable = boost::multi_index_container<
    T, boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
           boost::multi_index::const_mem_fun<Element, std::string_view,
                                             &Element::name_view>,
           std::hash<std::string_view>>>>;

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
  const std::string& id() const {
    return Role::role() == RoleSpecifier::kPublic ? Element::name()
                                                  : full_path_;
  }

  /// @returns The string view to the id to be used as a table key.
  std::string_view id_view() const { return id(); }

  /// @returns The string view to the unique full path for a table key.
  std::string_view full_path() const { return full_path_; }

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
  std::string full_path_;  ///< Unique for all elements per certain type.
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
        boost::multi_index::const_mem_fun<Id, std::string_view, &Id::id_view>,
        std::hash<std::string_view>>>>;

/// Wraps the element container tables into ranges of plain references
/// to hide the memory smart or raw pointers.
/// This is similar to boost::range::adaptors::indirect.
///
/// This range is used to enforce const correctness
/// by not leaking modifiable elements in the const containers of pointers.
///
/// @tparam T  Const or non-const associative container type.
template <class T>
class TableRange {
  /// Types inferred from the container.
  /// @{
  using deref_type = std::decay_t<decltype(*typename T::value_type(nullptr))>;
  using key_type = typename T::key_type;
  /// @}

 public:
  /// Value typedefs of the range.
  /// @{
  using value_type =
      std::conditional_t<std::is_const_v<T>, std::add_const_t<deref_type>,
                         deref_type>;
  using reference = value_type&;
  using pointer = value_type*;
  /// @}

  static_assert(std::is_const_v<T> == std::is_const_v<value_type>);

  /// The proxy forward iterator to extract values instead of pointers.
  class iterator : public boost::iterator_facade<iterator, value_type,
                                                 boost::forward_traversal_tag> {
    friend class boost::iterator_core_access;

   public:
    /// @param[in] it  The iterator of the T container.
    /*explicit*/ iterator(typename T::const_iterator it) : it_(it) {}  // NOLINT

   private:
    /// Standard forward iterator functionality.
    /// @{
    void increment() { ++it_; }
    bool equal(const iterator& other) const { return it_ == other.it_; }
    reference dereference() const { return **it_; }
    /// @}

    typename T::const_iterator it_;  ///< The iterator of value pointers.
  };

  using const_iterator = iterator;  ///< Phony const iterator for ranges.

  /// @param[in] table  The associative container of pointers.
  explicit TableRange(T& table) : table_(table) {}

  /// The proxy members for the common functionality of the associative table T.
  /// @{
  bool empty() const { return table_.empty(); }
  std::size_t size() const { return table_.size(); }
  std::size_t count(const key_type& key) const { return table_.count(key); }
  iterator find(const key_type& key) const { return table_.find(key); }
  iterator begin() const { return table_.begin(); }
  iterator end() const { return table_.end(); }
  iterator cbegin() const { return table_.begin(); }
  iterator cend() const { return table_.end(); }
  /// @}

 private:
  T& table_;  ///< The associative table being wrapped by this range.
};

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
  /// The key type of the table.
  using key_type = typename TableType::key_type;

  /// @returns The table as an associative range of type T elements.
  /// @{
  auto table() const { return TableRange(table_); }
  auto table() { return TableRange(table_); }
  /// @}

  /// Retrieves an element from the container.
  ///
  /// @param[in] id  The valid ID/name string of the element.
  ///
  /// @returns The reference to the element.
  ///
  /// @throws UndefinedElement  The element is not found.
  /// @{
  const T& Get(const key_type& id) const {
    auto it = table_.find(id);
    if (it != table_.end())
      return **it;

    SCRAM_THROW(UndefinedElement())
        << errinfo_element(std::string(id), T::kTypeString)
        << errinfo_container(Id::unique_name(static_cast<const Self&>(*this)),
                             Self::kTypeString);
  }
  T& Get(const key_type& id) {
    return const_cast<T&>(std::as_const(*this).Get(id));
  }
  /// @}

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
        SCRAM_THROW(UndefinedElement());
      if (&**it != element)
        SCRAM_THROW(LogicError("Duplicate element with different address."));

    } catch (Error& err) {
      err << errinfo_element(Id::unique_name(*element), T::kTypeString)
          << errinfo_container(Id::unique_name(static_cast<const Self&>(*this)),
                               Self::kTypeString);
      throw;
    }
    element->container(nullptr);
    return ext::extract(it, &table_);  // no-throw.
  }

 protected:
  /// @returns The data table with the elements.
  const TableType& data() const { return table_; }

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
/// @tparam T  The mef::Element type.
/// @tparam Ts  mef::Container types.
template <class T, class... Ts>
struct container_of {
  static_assert(std::is_base_of_v<Element, T>);

  using type = typename container_of_impl<T, Ts..., void>::type;  ///< Return.

  static_assert(!std::is_same_v<type, void>,
                "No container with elements of type T.");
};

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
  /// @returns The table as an associative range of type T elements.
  /// @{
  template <class T>
  auto table() const {
    using ContainerType = typename detail::container_of<T, Ts...>::type;
    return ContainerType::table();
  }
  template <class T>
  auto table() {
    using ContainerType = typename detail::container_of<T, Ts...>::type;
    return ContainerType::table();
  }
  /// @}

  /// Retrieves an element from the container.
  ///
  /// @tparam T  The mef::Element type in the composite container.
  ///
  /// @param[in] id  The valid ID/name string of the element.
  ///
  /// @returns The reference to the element.
  ///
  /// @throws UndefinedElement  The element is not found.
  /// @{
  template <class T,
            class ContainerType = typename detail::container_of<T, Ts...>::type>
  const T& Get(const typename ContainerType::key_type& id) const {
    return ContainerType::Get(id);
  }
  template <class T,
            class ContainerType = typename detail::container_of<T, Ts...>::type>
  T& Get(const typename ContainerType::key_type& id) {
    return ContainerType::Get(id);
  }
  /// @}

 protected:
  /// @returns The data table with elements of specific type.
  template <class T>
  decltype(auto) data() const {
    using ContainerType = typename detail::container_of<T, Ts...>::type;
    return ContainerType::data();
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
