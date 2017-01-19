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

/// @file index_map.h
/// Non-zero based Index->Value map adapter on sequential containers.

#ifndef SCRAM_SRC_EXT_INDEX_MAP_H_
#define SCRAM_SRC_EXT_INDEX_MAP_H_

#include <vector>

namespace ext {

/// An adaptor map to shift zero-based containers to a different base.
///
/// @tparam BaseIndex  The starting index for the container adaptor.
/// @tparam T  A value type for the underlying container.
/// @tparam Sequence  A zero-based sequence container supporting operator[].
///
/// @post The adaptor only guarantees access adjustment for operator[].
///       The element access by other means are not adjusted, e.g., at().
template <std::size_t BaseIndex, typename T,
          template <typename...> class Sequence = std::vector>
class index_map : public Sequence<T> {
 public:
  using Sequence<T>::Sequence;

  /// @returns The reference to value at the index.
  ///
  /// @param[in] index  The BaseIndex-based index of the container adaptor.
  ///
  /// @pre index >= BaseIndex
  ///
  /// @{
  typename Sequence<T>::reference operator[](std::size_t index) {
    return Sequence<T>::operator[](index - BaseIndex);
  }
  typename Sequence<T>::const_reference operator[](std::size_t index) const {
    return Sequence<T>::operator[](index - BaseIndex);
  }
  /// @}
};

}  // namespace ext

#endif  // SCRAM_SRC_EXT_INDEX_MAP_H_
