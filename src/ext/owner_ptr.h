/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

#ifndef SCRAM_SRC_EXT_OWNER_PTR_H_
#define SCRAM_SRC_EXT_OWNER_PTR_H_

/// @file owner_ptr.h
/// Single ownership pointer complementary to unique_ptr.

#include <cassert>
#include <cstdint>

#include <memory>
#include <type_traits>
#include <utility>

namespace ext {

/// Single ownership smart pointer
/// that allows ownership transfer
/// without pointer invalidation (unlike unique_ptr).
///
/// This is a minimal implementation
/// to emulate 'extract' functionality from associative and set containers.
/// This functionality can be worked around
/// with a custom deleter for unique_ptr;
/// however, it doubles the size of the pointer.
template <typename T, typename = std::enable_if_t<!std::is_array<T>::value>>
class owner_ptr final {
 public:
  using pointer = T*;  ///< Needs plain pointers for bit-packing optimization.
  using reference = T&;  ///< The expected result of pointer dereference.

  /// Constructs null/empty pointer.
  ///
  /// @post nullptr is never owned.
  /// @{
  constexpr owner_ptr() noexcept : data_(0) {}
  constexpr owner_ptr(std::nullptr_t) noexcept : owner_ptr() {}
  /// @}

  /// Takes ownership of a pointer.
  explicit owner_ptr(pointer ptr) noexcept
      : data_(ptr ? reinterpret_cast<std::uintptr_t>(ptr) | kOwnerMask : 0) {}

  /// Transfers the pointer and ownership information.
  /// @{
  owner_ptr(owner_ptr&& other) noexcept : data_(other.data_) { other.disown(); }
  explicit owner_ptr(std::unique_ptr<T>&& other) noexcept
      : owner_ptr(other.release()) {}
  operator std::unique_ptr<T>() && noexcept {
    return std::unique_ptr<T>(release());
  }
  owner_ptr& operator=(owner_ptr&& other) noexcept {
    this->~owner_ptr();
    new(this) owner_ptr(std::move(other));
    return *this;
  }
  owner_ptr& operator=(std::unique_ptr<T>&& other) noexcept {
    reset(other.release());
    return *this;
  }
  owner_ptr& operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }
  /// @}

  /// Deletes the pointer only if it belongs to this owner.
  ~owner_ptr() noexcept {
    if (*this)
      delete get();
  }

  /// Dereference the stored pointer.
  ///
  /// @returns The reference to the pointed object.
  ///
  /// @pre The stored pointer is not nullptr.
  /// @pre If the owner has changed (i.e., it's been moved),
  ///      the pointed object must still be alive
  ///      (this is the main difference from unique_ptr).
  reference operator*() const { return *get(); }

  /// @returns The stored pointer.
  /// @{
  pointer operator->() const noexcept { return get(); }
  pointer get() const noexcept {
    return reinterpret_cast<pointer>(data_ & ~kOwnerMask);
  }
  /// @}

  /// @returns true if this is the owner (dereferencing is guaranteed).
  ///
  /// @note This is different from unique_ptr.
  ///       If nullptr test is needed for sure,
  ///       test the get() directly.
  explicit operator bool() const noexcept {
    return data_ & kOwnerMask;  // nullptr is never owned.
  }

  /// Releases the ownership.
  ///
  /// @returns nullptr if this is not the owner.
  ///
  /// @post get() yields the original pointer (not reset to nullptr).
  pointer release() noexcept {
    if (*this) {
      disown();
      return reinterpret_cast<pointer>(data_);
    }
    return nullptr;
  }

  /// Resets the owned pointer.
  void reset(pointer ptr = nullptr) noexcept {
    this->~owner_ptr();
    new(this) owner_ptr(ptr);
  }

  /// Swaperator.
  void swap(owner_ptr& other) noexcept { std::swap(data_, other.data_); }

 private:
  static const std::uintptr_t kOwnerMask = 1;  ///< The owner bit location.

  /// Ceases the pointer ownership.
  void disown() noexcept { data_ &= ~kOwnerMask; }

  std::uintptr_t data_;  ///< The pointer and owner bit are packed.
};

/// Non-member swaperator for owner pointers.
template <typename T>
void swap(owner_ptr<T>& lhs, owner_ptr<T>& rhs) {
  lhs.swap(rhs);
}

}  // namespace ext

#endif  // SCRAM_SRC_EXT_OWNER_PTR_H_
