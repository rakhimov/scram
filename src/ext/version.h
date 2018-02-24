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
/// String version interpretation facilities.

#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace ext {

/// Converts a (dotted) version string to version numbers.
///
/// @param[in] version  The numeric version string in specific (dotted) format.
/// @param[in] separator The separator character for the version format.
///
/// @returns (major, minor, micro) versions extracted from the string.
///          none if the version string cannot be interpreted into numbers.
///
/// @post If minor or micro number is missing, it is 0 (i.e., no failure).
std::optional<std::array<int, 3>> extract_version(std::string_view version,
                                                  char separator = '.');

}  // namespace ext
