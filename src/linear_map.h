/*
 * Copyright (C) 2016 Olzhas Rakhimov
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

/// @file linear_map.h
/// Implementation of a vector-based map for a small number of entries.

#ifndef SCRAM_SRC_LINEAR_MAP_H_
#define SCRAM_SRC_LINEAR_MAP_H_

#include <cassert>

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <utility>

namespace scram {

/// Default erase policy for containers with iterators.
struct DefaultEraser {
  /// Erases an element from a container with its default ``erase`` API.
  ///
  /// @tparam T  The container type.
  /// @tparam Iterator  The iterator type belonging to the container.
  ///
  /// @param[in] it  The iterator pointing to the element.
  /// @param[in,out] container  The host container.
  ///
  /// @returns The iterator as the result of call to the container's ``erase``.
  ///
  /// @{
  template <class T>
  static typename T::iterator erase(typename T::iterator it, T* container) {
    return container->erase(it);
  }
  template <class T>  // Workaround for the C++11 bug in libstdc++ 4.8.
  static typename T::iterator erase(typename T::const_iterator it,
                                    T* container) {
    return DefaultEraser::erase(
        std::next(container->begin(), std::distance(container->cbegin(), it)),
        container);
  }
  /// @}
};

/// Erase policy based on moving the last element to the erased element.
struct MoveEraser {
  /// Moves the last element into the to-be-erased element.
  /// Then, the last element is popped back.
  /// This is an efficient, constant time operation for contiguous containers.
  ///
  /// @tparam T  The container type.
  ///
  /// @param[in] it  The iterator pointing to the element.
  /// @param[in,out] container  The host container.
  ///
  /// @returns The iterator pointing to the original position.
  ///
  /// @warning The order of elements is changed after this erase.
  ///
  /// @{
  template <class T>
  static typename T::iterator erase(typename T::iterator it, T* container) {
    if (it != std::prev(container->end())) {  // Prevent move into itself.
      *it = std::move(container->back());
    }
    container->pop_back();
    return it;
  }
  template <class T>
  static typename T::iterator erase(typename T::const_iterator it,
                                    T* container) {
    return MoveEraser::erase(
        std::next(container->begin(), std::distance(container->cbegin(), it)),
        container);
  }
  /// @}
};

/// An adaptor map with lookup complexity O(N)
/// based on sequence (contiguous structure by default).
/// This map is designed for a small number of elements
/// and for small <Key, Value> size pairs.
/// Consider this class a convenient wrapper
/// around std::vector<std::pair<Key, Value>>.
///
/// Since this map is based on the vector by default,
/// the order of insertions is preserved,
/// and it provides random access iterators.
///
/// The major differences from the standard library maps:
///
///   1. The entry is std::pair<Key, Value>
///      instead of std::pair<const Key, Value>,
///      which means that the key can be modified
///      as long as it stays unique.
///
///   2. Iterators, references, pointers can be invalidated
///      by modifier functions (insert, erase, reserve, etc.).
///      This is the inherited behavior from std::vector.
///
///   3. Some API may be extra or missing.
///
/// The performance of the map critically depends on the number of entries,
/// the size of the key-value pair, and the cost of comparing keys for equality.
/// The advantage of the LinearMap comes from cache-friendliness,
/// and fewer CPU front-end and back-end stalls.
///
/// From crude experimental results with random entries
/// comparing with std::map, std::unordered_map, boost::flat_map:
///
///   1. For Key=int, Value=Object, sizeof(Object)=24,
///      the LinearMap outperforms up to 50 entries.
///
///   2. For Key=std::string(20 char), Value=Object, sizeof(Object)=24,
///      the LinearMap performs equally well up to 10 entries.
///
/// @tparam Key  The type of the unique keys.
/// @tparam Value  The type of the values associated with the keys.
/// @tparam Sequence  The underlying container type.
/// @tparam ErasePolicy  The policy class that provides
///                      ``erase(it, *container)`` static member function
///                      to control the element erasure from the container.
template <typename Key, typename Value,
          template <typename...> class Sequence = std::vector,
          class ErasePolicy = DefaultEraser>
class LinearMap {
  /// Non-member equality test operators.
  /// The complexity is O(N^2).
  ///
  /// @param[in] lhs  First map.
  /// @param[in] rhs  Second map.
  ///
  /// @note The order of elements is not relevant.
  ///       If the order matters for equality,
  ///       compare the underlying data containers directly.
  ///
  /// @{
  friend bool operator==(const LinearMap& lhs, const LinearMap& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (const auto& entry : lhs) {
       if (std::find(rhs.begin(), rhs.end(), entry) == rhs.end()) return false;
    }
    return true;
  }
  friend bool operator!=(const LinearMap& lhs, const LinearMap& rhs) {
    return !(lhs == rhs);
  }
  /// @}

  /// Friend swap definition for convenience sake.
  friend void swap(LinearMap& lhs, LinearMap& rhs) noexcept { lhs.swap(rhs); }

 public:
  /// Public typedefs.
  /// @{
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<key_type, mapped_type>;
  using container_type = Sequence<value_type>;
  /// @}

  /// Iterator-related typedefs.
  /// @{
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;
  using reverse_iterator = typename container_type::reverse_iterator;
  using const_reverse_iterator =
      typename container_type::const_reverse_iterator;
  using size_type = typename container_type::size_type;
  using difference_type = typename container_type::difference_type;
  /// @}

  /// Standard constructors and assignment operators.
  /// @{
  LinearMap() = default;
  LinearMap(std::initializer_list<value_type> init_list) {
    LinearMap::insert(init_list.begin(), init_list.end());
  }

  template <typename Iterator>
  LinearMap(Iterator first1, Iterator last1) {
    LinearMap::insert(first1, last1);
  }

  LinearMap(const LinearMap& lm) : map_(lm.map_) {}
  LinearMap(LinearMap&& lm) noexcept : map_(std::move(lm.map_)) {}

  LinearMap& operator=(const LinearMap& lm) {
    map_ = lm.map_;
    return *this;
  }
  LinearMap& operator=(LinearMap&& lm) noexcept {
    assert(this != &lm && "Move into itself is undefined.");
    map_ = std::move(lm.map_);
    return *this;
  }
  /// @}

  /// Finds an entry in the map.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns Iterator pointing to the entry,
  ///          or end() if not found.
  /// @{
  const_iterator find(const key_type& key) const {
    return std::find_if(map_.cbegin(), map_.cend(),
                        [&key](const value_type& p) { return p.first == key; });
  }

  iterator find(const key_type& key) {
    return std::find_if(map_.begin(), map_.end(),
                        [&key](const value_type& p) { return p.first == key; });
  }
  /// @}

  /// Determines if an entry with the given key in the map.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns 1 if there's an entry,
  ///          0 otherwise.
  size_type count(const key_type& key) const {
    return LinearMap::find(key) != map_.end();
  }

  /// Accesses an existing or default constructed entry.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns A reference to the value of the entry.
  ///
  /// @{
  mapped_type& operator[](const key_type& key) {
    auto it = LinearMap::find(key);
    if (it != map_.end()) return it->second;
    map_.emplace_back(key, mapped_type());
    return std::prev(map_.end())->second;
  }

  mapped_type& operator[](key_type&& key) {
    auto it = LinearMap::find(key);
    if (it != map_.end()) return it->second;
    map_.emplace_back(std::move(key), mapped_type());
    return std::prev(map_.end())->second;
  }
  /// @}

  /// Accesses the value of the entry.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns The reference to the value.
  ///
  /// @throws std::out_of_range  The entry is not in the map.
  ///
  /// @{
  const mapped_type& at(const key_type& key) const {
    auto it = LinearMap::find(key);
    if (it == map_.end()) throw std::out_of_range("Key is not found.");
    return it->second;
  }

  mapped_type& at(const key_type& key) {
    return const_cast<mapped_type&>(
        static_cast<const LinearMap*>(this)->at(key));
  }
  /// @}

  /// Inserts a key-value pair into the map
  /// if the pair is not in the map.
  ///
  /// @param[in] p  The entry.
  ///
  /// @returns A pair of an iterator and insertion flag.
  ///          The iterator points to possibly inserted entry,
  ///          and the flag indicates whether the entry is actually inserted.
  /// @{
  std::pair<iterator, bool> insert(const value_type& p) {
    auto it = LinearMap::find(p.first);
    if (it != map_.end()) return {it, false};
    map_.push_back(p);
    return {std::prev(map_.end()), true};
  }

  std::pair<iterator, bool> insert(value_type&& p) {
    auto it = LinearMap::find(p.first);
    if (it != map_.end()) return {it, false};
    map_.emplace_back(std::forward<value_type>(p));
    return {std::prev(map_.end()), true};
  }
  /// @}

  /// Inserts a range of elements.
  /// The range is not assumed to be unique.
  ///
  /// @tparam Iterator  Iterator to the container with key-value pairs.
  ///
  /// @param[in] first1  The beginning of the range.
  /// @param[in] last1  The end of the range.
  template <typename Iterator>
  void insert(Iterator first1, Iterator last1) {
    for (; first1 != last1; ++first1) {
      if (LinearMap::find(first1->first) == map_.end())
        map_.push_back(*first1);
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
    value_type p(std::forward<Ts>(args)...);
    auto it = LinearMap::find(p.first);
    if (it != map_.end()) return {it, false};
    map_.emplace_back(std::move(p));
    return {std::prev(map_.end()), true};
  }

  /// Erases the entry pointed by an iterator.
  ///
  /// @param[in] pos  An iterator pointing to the entry.
  ///
  /// @returns An iterator pointing after the entry.
  ///
  /// @{
  iterator erase(const_iterator pos) { return ErasePolicy::erase(pos, &map_); }
  iterator erase(iterator pos) { return ErasePolicy::erase(pos, &map_); }
  /// @}

  /// Erases the entry with a key.
  ///
  /// @param[in] key  The key of the entry.
  ///
  /// @returns 1 if the existing entry has been removed,
  ///          0 if there's no entry with the given key.
  size_type erase(const key_type& key) {
    iterator it = LinearMap::find(key);
    if (it == map_.end()) return 0;
    LinearMap::erase(it);
    return 1;
  }

  /// Swaps data with another linear map.
  ///
  /// @param[in] other  Another linear map.
  void swap(LinearMap& other) noexcept { map_.swap(other.map_); }

  /// @returns The number of entries in the map.
  size_type size() const { return map_.size(); }

  /// @returns true if there are no entries.
  bool empty() const { return map_.empty(); }

  /// Erases all entries in the map.
  void clear() noexcept { map_.clear(); }

  /// Prepares the linear map for a specified number of entries.
  ///
  /// @param[in] n  The number of expected entries.
  void reserve(size_type n) { map_.reserve(n); }

  /// @returns The capacity of the underlying container.
  size_type capacity() const { return map_.capacity(); }

  /// @returns The underlying data container.
  ///          The container elements are ordered exactly as inserted.
  const container_type& data() const { return map_; }

  /// @returns A read/write iterator pointing to the first entry.
  iterator begin() { return map_.begin(); }
  /// @returns A read/write iterator pointing one past the last entry.
  iterator end() { return map_.end(); }

  /// @returns A read-only iterator pointing to the first entry.
  ///
  /// @{
  const_iterator cbegin() const { return map_.cbegin(); }
  const_iterator begin() const { return map_.cbegin(); }
  /// @}

  /// @returns A read-only iterator pointing one past the last entry.
  ///
  /// @{
  const_iterator cend() const { return map_.cend(); }
  const_iterator end() const { return map_.cend(); }
  /// @}

  /// Corresponding reverse iterators.
  ///
  /// @{
  reverse_iterator rbegin() { return map_.rbegin(); }
  reverse_iterator rend() { return map_.rend(); }
  const_reverse_iterator crbegin() const { return map_.crbegin(); }
  const_reverse_iterator rbegin() const { return map_.crbegin(); }
  const_reverse_iterator crend() const { return map_.crend(); }
  const_reverse_iterator rend() const { return map_.crend(); }
  /// @}

 private:
  container_type map_;  ///< The main underlying data container.
};

}  // namespace scram

#endif  // SCRAM_SRC_LINEAR_MAP_H_
