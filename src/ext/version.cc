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
/// Out-of-line implementations of version interpretation facilities.

#include "src/ext/version.h"

namespace ext {

std::optional<std::array<int, 3>> extract_version(std::string_view version,
                                                  char separator) {
  if (version.empty())
    return {};

  std::array<int, 3> numbers{};
  int i = 0;  // Version number position.
  bool expect_separator = false;  // Consecutive separators are not acceptable.

  for (char c : version) {
    if (c == separator) {
      if (!expect_separator)
        return {};
      if (++i == numbers.size())
        return {};
      expect_separator = false;

    } else if (c < '0' || c > '9') {
      return {};

    } else {
      numbers[i] *= 10;
      numbers[i] += c - '0';
      expect_separator = true;
    }
  }
  return numbers;
}

}  // namespace ext
