/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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
/// Extra helper functions for std::variant.

#pragma once

#include <variant>

namespace ext {

/// Retrieves the stored value in the variant with a common type.
///
/// @tparam T  The type compatible with any type stored in the variant.
/// @tparam Ts  The types of values stored in the variant.
///
/// @param[in] var  The variant with argument values.
///
/// @returns The stored value cast to the type T.
template <typename T, typename... Ts>
T as(const std::variant<Ts...>& var) {
  return std::visit([](auto& arg) { return static_cast<T>(arg); }, var);
}

/// Workaround gcc 8 bug with std::variant swap.
///
/// @param[in,out] lhs
/// @param[in,out] rhs
///
/// @todo Require patched GCC 8 instead of this workaround.
template <typename... Ts>
void swap(std::variant<Ts...>& lhs, std::variant<Ts...>& rhs) noexcept {
#if __GNUC__ == 8 && __GNUC_MINOR__ < 3
  auto tmp = std::move(rhs);
  rhs = std::move(lhs);
  lhs = std::move(tmp);
#else
  lhs.swap(rhs);
#endif
}

}  // namespace ext
