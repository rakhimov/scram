/*
 * Copyright (C) 2015-2017 Olzhas Rakhimov
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

/// @file main.cpp
/// The proxy main() function
/// with no link dependencies except for the SCRAM GUI (shared lib).
/// The reason for this separation
/// is weird linker failures on Windows with shared libraries.

/// The actual entrance to the SCRAM GUI.
extern int launchGui(int, char*[]);

int main(int argc, char *argv[]) { return launchGui(argc, argv); }
