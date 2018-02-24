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
/// Project/configuration management facilities
/// to make various setups for analysis possible.

#pragma once

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "settings.h"
#include "xml.h"

namespace scram {

/// This class processes project configuration files for analysis.
/// The class contains all the setup and state
/// to initialize general analysis.
class Project {
 public:
  /// A constructor with project configurations for analysis.
  /// Reads and validates the configurations.
  ///
  /// All relative paths in the configuration are resolved
  /// with respect to the location of the original configuration file.
  ///
  /// @param[in] project_file  The path to the project file with configurations.
  ///
  /// @throws VersionError  The version required by the file is incompatible.
  /// @throws ValidityError  The configurations have problems.
  /// @throws SettingsError  Settings values contain errors.
  /// @throws IOError  The file is not accessible.
  explicit Project(const std::string& project_file);

  /// @returns normalized, absolute paths to input files for analysis.
  const std::vector<std::string>& input_files() const { return input_files_; }

  /// @returns the settings for analysis.
  const core::Settings& settings() const { return settings_; }

 private:
  /// Gathers input files with analysis constructs.
  ///
  /// @param[in] root  The root XML element.
  /// @param[in] base_path  The base path for relative path resolution.
  void GatherInputFiles(const xml::Element& root,
                        const boost::filesystem::path& base_path);

  /// Gathers options for analysis.
  ///
  /// @param[in] root  The root XML element.
  void GatherOptions(const xml::Element& root);

  /// Extracts analysis types to be performed from analysis element.
  ///
  /// @param[in] analysis  Analysis element node.
  void SetAnalysis(const xml::Element& analysis);

  /// Extracts limits for analysis.
  ///
  /// @param[in] limits  An XML element containing various limits.
  void SetLimits(const xml::Element& limits);

  /// Container for input files for analysis.
  /// These input files contain fault trees, events, etc.
  std::vector<std::string> input_files_;
  core::Settings settings_;  ///< Settings for specific analysis.
};

}  // namespace scram
