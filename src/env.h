/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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
/// SCRAM-specific environment variables.
///
/// All paths are absolute, canonical, and POSIX (with '/' separator).
///
/// @pre The system follows the Filesystem Hierarchy Standard.

#pragma once

#include <string>

namespace scram::env {

/// @returns The location of the RELAX NG schema for project files.
const std::string& project_schema();

/// @returns The location of the RELAX NG schema for input files.
const std::string& input_schema();

/// @returns The location of the RELAX NG schema for output report files.
const std::string& report_schema();

/// @returns The path to the installation directory.
const std::string& install_dir();

}  // namespace scram::env
