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

/// @file version.h
/// Set of functions with version information
/// of the core and dependencies.

#ifndef SCRAM_SRC_VERSION_H_
#define SCRAM_SRC_VERSION_H_

namespace scram {
namespace version {

/// @returns Git generated tag recent version.
const char* describe();

/// @returns Build type.
const char* build();

/// @returns The core version.
const char* core();

/// @returns The version of the boost.
const char* boost();

/// @returns The version of XML libraries.
const char* xml();

}  // namespace version
}  // namespace scram

#endif  // SCRAM_SRC_VERSION_H_
