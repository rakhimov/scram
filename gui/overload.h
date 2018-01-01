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

/// @file
/// Facilities to help with pointers to overloaded functions.

/// Helper macro to resolve overloaded signals in Qt 5 style connections.
///
/// @param T  The class type of the signal.
/// @param name  The name of the signal function.
/// @param ...  The parameter types of the signal function.
#define OVERLOAD(T, name, ...) static_cast<void (T::*)(__VA_ARGS__)>(&T::name)
