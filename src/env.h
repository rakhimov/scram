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

/// @file env.h
/// Environmental Variables

#ifndef SCRAM_SRC_ENV_H_
#define SCRAM_SRC_ENV_H_

#include <string>

namespace scram {

/// Provides environmental variables.
class Env {
 public:
  /// @returns The location of the RELAX NG schema for configuration files.
  static std::string config_schema();

  /// @returns The location of the RELAX NG schema for input files.
  static std::string input_schema();

  /// @returns The location of the RELAX NG schema for output report files.
  static std::string report_schema();

  /// @returns The path to the installation directory.
  static const std::string& install_dir() { return kInstallDir_; }

 private:
  static const std::string kInstallDir_;  ///< Installation directory.
};

}  // namespace scram

#endif  // SCRAM_SRC_ENV_H_
