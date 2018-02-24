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
/// The environment variables discovered at run-time.

#include "env.h"

#include <boost/dll/runtime_symbol_info.hpp>

namespace scram::env {

const std::string& project_schema() {
  static const std::string schema_path =
      install_dir() + "/share/scram/project.rng";
  return schema_path;
}

const std::string& input_schema() {
  static const std::string schema_path =
      install_dir() + "/share/scram/input.rng";
  return schema_path;
}

const std::string& report_schema() {
  static const std::string schema_path =
      install_dir() + "/share/scram/report.rng";
  return schema_path;
}

const std::string& install_dir() {
  static const std::string install_path =
      boost::dll::program_location()  // executable
          .parent_path()  // bin
          .parent_path()  // install
          .generic_string();  // POSIX format.
  return install_path;
}

}  // namespace scram::env
