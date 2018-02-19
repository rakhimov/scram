/*
 * Copyright (C) 2018 Olzhas Rakhimov
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
/// Implementation of a vector-based set for a small number of entries.

#pragma once

#include <algorithm>
#include <initializer_list>
#include <utility>
#include <vector>

namespace ext {

/// Identity functor.
struct identity {
  /// Identity for non-rvalue objects only.
  template <typename T>
  T& operator()(T& arg) const {  // NOLINT
    return arg;
  }
};

/// An adaptor set with lookup complexity O(N)
/// based on sequence (contiguous structure by default).
/// This set is designed for a small number of elements.
/// Consider this class a convenient wrapper around std::vector<Value>.
///
/// Since this set is based on the vector by default,
/// the order of insertions is preserved,
/// and it provides random access iterators.
///
/// Unlike STL sets,
/// this set is designed like boost::multi_index or Python sets;
/// that is, there is key retrieval (indexation) policy for values.
/// Therefore, this set also acts like a map.
///
/// The major differences from the standard library sets:
///
///   0. Values are not const.
///
///   1. Iterators, references, pointers can be invalidated
///      by modifier functions (insert, erase, reserve, etc.).
///      This is the inherited behavior from std::vector.
///
///   2. Some API may be extra or missing.
///
///   3. Key extraction is not stored as member data.
///      Instead, it is treated as a policy.
///
/// The performance of the set critically depends on the number of entries,
/// the size of the value, and the cost of comparing values for equality.
/// The advantage of the linear_set comes from cache-friendliness,
/// and fewer CPU front-end and back-end stalls.
///
/// @tparam Value  The type of the values.
/// @tparam KeyFromValue  The key extraction functor.
/// @tparam Sequence  The underlying container type.
template <typename Value, typename KeyFromValue = identity,
          template <typename...> class Sequence = std::vector>
class linear_set {
  /// Non-member equality test operators.
  /// The complexity is O(N^2).
  ///
  /// @param[in] lhs  First set.
  /// @param[in] rhs  Second set.
  ///
  /// @note The order of elements is not relevant.
  ///       If the order matters for equality,
  ///       compare the underlying data containers directly.
  ///
  /// @{
  friend bool operator==(const linear_set& lhs, const linear_set& rhs) {
    if (lhs.size() != rhs.size())
      return false;
    return std::all_of(lhs.begin(), lhs.end(), [&rhs](const Value& value) {
      return rhs.count_value(value);
    });
  }
  friend bool operator!=(const linear_set& lhs, const linear_set& rhs) {
    return !(lhs == rhs);
  }
  /// @}

  /// Friend swap definition for convenience sake.
  friend void swap(linear_set& lhs, linear_set& rhs) noexcept { lhs.swap(rhs); }

 public:
  /// Public typedefs similar to standard sets.
  /// @{
  using key_from_value = KeyFromValue;
  using value_type = Value;
  using key_type = std::decay_t<decltype(
      key_from_value()(*static_cast<value_type*>(nullptr)))>;
  using container_type = Sequence<value_type>;
  /// @}

  /// Iterator-related typedefs redeclared from the underlying container type.
  /// @{
  using pointer = typename container_type::pointer;
  using const_pointer = typename container_type::const_pointer;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;
  using reverse_iterator = typename container_type::reverse_iterator;
  using const_reverse_iterator =
      typename container_type::const_reverse_iterator;
  using size_type = typename container_type::size_type;
  using difference_type = typename container_type::difference_type;
  /// @}

  /// Standard constructors (copy and move are implicit).
  /// @{
  linear_set() = default;
  linear_set(std::initializer_list<value_type> init_list) {
    linear_set::insert(init_list.begin(), init_list.end());
  }

  template <typename Iterator>
  linear_set(Iterator first1, Iterator last1) {
    linear_set::insert(first1, last1);
  }
  /// @}

  /// @returns Value-to-key converter.
  key_from_value key_extractor() const { return key_from_value(); }

  /// Finds an entry in the set by key.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns Iterator pointing to the entry,
  ///          or end() if not found.
  /// @{
  const_iterator find(const key_type& key) const {
    return std::find_if(set_.cbegin(), set_.cend(),
                        [&key](const value_type& value) {
                          return key == key_from_value()(value);
                        });
  }

  iterator find(const key_type& key) {
    return std::find_if(set_.begin(), set_.end(),
                        [&key](const value_type& value) {
                          return key == key_from_value()(value);
                        });
  }
  /// @}

  /// Determines if an entry in the set.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns 1 if there's an entry,
  ///          0 otherwise.
  size_type count(const key_type& key) const {
    return linear_set::find(key) != set_.end();
  }

  /// Inserts a value into the set
  /// if the equal value is not in the set.
  ///
  /// @param[in] value  The new value to be inserted.
  ///
  /// @returns A pair of an iterator and insertion flag.
  ///          The iterator points to possibly inserted entry,
  ///          and the flag indicates whether the entry is actually inserted.
  /// @{
  std::pair<iterator, bool> insert(const value_type& value) {
    auto it = linear_set::find_value(value);
    if (it != set_.end())
      return {it, false};
    set_.push_back(value);
    return {std::prev(set_.end()), true};
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    auto it = linear_set::find_value(value);
    if (it != set_.end())
      return {it, false};
    set_.emplace_back(std::forward<value_type>(value));
    return {std::prev(set_.end()), true};
  }
  /// @}

  /// Inserts a range of elements.
  /// The range is not assumed to be unique.
  ///
  /// @tparam Iterator  Iterator to the container with values.
  ///
  /// @param[in] first1  The beginning of the range.
  /// @param[in] last1  The end of the range.
  template <typename Iterator>
  void insert(Iterator first1, Iterator last1) {
    for (; first1 != last1; ++first1) {
      if (!linear_set::count_value(*first1))
        set_.push_back(*first1);
    }
  }

  /// Attempts to build and insert an entry.
  ///
  /// @param[in] args  Arguments for the construction of the entry.
  ///
  /// @returns An iterator pointing to the entry,
  ///          and a flag indicating if the insertion actually happened.
  template <typename... Ts>
  std::pair<iterator, bool> emplace(Ts&&... args) {
    value_type value(std::forward<Ts>(args)...);
    auto it = linear_set::find_value(value);
    if (it != set_.end())
      return {it, false};
    set_.emplace_back(std::move(value));
    return {std::prev(set_.end()), true};
  }

  /// Erases the entry pointed by an iterator.
  ///
  /// @param[in] pos  An iterator pointing to the entry.
  ///
  /// @returns An iterator pointing after the entry.
  ///
  /// @{
  iterator erase(const_iterator pos) { return set_.erase(pos); }
  iterator erase(iterator pos) { return set_.erase(pos); }
  /// @}

  /// Erases the entry with a key.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns 1 if the existing entry has been removed,
  ///          0 if there's no entry with the given key.
  size_type erase(const key_type& key) {
    iterator it = linear_set::find(key);
    if (it == set_.end())
      return 0;
    linear_set::erase(it);
    return 1;
  }

  /// Swaps data with another linear set.
  ///
  /// @param[in] other  Another linear set.
  void swap(linear_set& other) noexcept { set_.swap(other.set_); }

  /// @returns The number of entries in the set.
  size_type size() const { return set_.size(); }

  /// @returns true if there are no entries.
  bool empty() const { return set_.empty(); }

  /// Erases all entries in the set.
  void clear() noexcept { set_.clear(); }

  /// Prepares the linear set for a specified number of entries.
  ///
  /// @param[in] n  The number of expected entries.
  void reserve(size_type n) { set_.reserve(n); }

  /// @returns The capacity of the underlying container.
  size_type capacity() const { return set_.capacity(); }

  /// @returns The underlying data container.
  ///          The container elements are ordered exactly as inserted.
  /// @{
  container_type& data() { return set_; }
  const container_type& data() const { return set_; }
  /// @}

  /// @returns A read/write iterator pointing to the first entry.
  iterator begin() { return set_.begin(); }
  /// @returns A read/write iterator pointing one past the last entry.
  iterator end() { return set_.end(); }

  /// @returns A read-only iterator pointing to the first entry.
  ///
  /// @{
  const_iterator cbegin() const { return set_.cbegin(); }
  const_iterator begin() const { return set_.cbegin(); }
  /// @}

  /// @returns A read-only iterator pointing one past the last entry.
  ///
  /// @{
  const_iterator cend() const { return set_.cend(); }
  const_iterator end() const { return set_.cend(); }
  /// @}

  /// Corresponding reverse iterators.
  ///
  /// @{
  reverse_iterator rbegin() { return set_.rbegin(); }
  reverse_iterator rend() { return set_.rend(); }
  const_reverse_iterator crbegin() const { return set_.crbegin(); }
  const_reverse_iterator rbegin() const { return set_.crbegin(); }
  const_reverse_iterator crend() const { return set_.crend(); }
  const_reverse_iterator rend() const { return set_.crend(); }
  /// @}

 private:
  /// Finds an entry in the set by value.
  ///
  /// @param[in] value  The value of the entry.
  ///
  /// @returns Iterator pointing to the entry with an equivalent value,
  ///          or end() if not found.
  /// @{
  const_iterator find_value(const value_type& value) const {
    return find(key_from_value()(value));
  }
  iterator find_value(const value_type& value) {
    return find(key_from_value()(value));
  }
  /// @}

  /// Determines if an entry in the set by value.
  ///
  /// @param[in] value  The value of the entry.
  ///
  /// @returns 1 if there's an entry,
  ///          0 otherwise.
  size_type count_value(const value_type& value) const {
    return linear_set::find_value(value) != set_.end();
  }

  container_type set_;  ///< The main underlying data container.
};

}  // namespace ext
