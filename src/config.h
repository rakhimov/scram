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

/// @file config.h
/// Configuration management facilities
/// to make various setups for analysis possible.

#ifndef SCRAM_SRC_CONFIG_H_
#define SCRAM_SRC_CONFIG_H_

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <libxml++/libxml++.h>

#include "settings.h"

namespace scram {

/// This class processes configuration files for analysis.
/// The class contains all the setup and state
/// to initialize general analysis.
class Config {
 public:
  /// A constructor with configurations for analysis.
  /// Reads and validates the configurations.
  ///
  /// All relative paths in the configuration are resolved
  /// with respect to the location of the original configuration file.
  ///
  /// @param[in] config_file  The path to an XML file with configurations.
  ///
  /// @throws ValidationError  The configurations have problems.
  /// @throws InvalidArgument  Settings values contain errors.
  /// @throws IOError  The file is not accessible.
  explicit Config(const std::string& config_file);

  /// @returns normalized, absolute paths to input files for analysis.
  const std::vector<std::string>& input_files() const { return input_files_; }

  /// @returns the settings for analysis.
  const core::Settings& settings() const { return settings_; }

  /// @returns the output destination path (absolute, normalized).
  /// @returns empty string if no path has been set.
  const std::string& output_path() const { return output_path_; }

 private:
  /// Gathers input files with analysis constructs.
  ///
  /// @param[in] root  The root XML element.
  /// @param[in] base_path  The base path for relative path resolution.
  void GatherInputFiles(const xmlpp::Node* root,
                        const boost::filesystem::path& base_path);

  /// Gathers options for analysis.
  ///
  /// @param[in] root  The root XML element.
  void GatherOptions(const xmlpp::Node* root);

  /// Gets the output path for reports.
  ///
  /// @param[in] root  The root XML element.
  /// @param[in] base_path  The base path for relative path resolution.
  void GetOutputPath(const xmlpp::Node* root,
                     const boost::filesystem::path& base_path);

  /// Extracts Qualitative analysis algorithm.
  ///
  /// @param[in] analysis  Analysis element node.
  void SetAlgorithm(const xmlpp::Element* analysis);

  /// Extracts analysis types to be performed from analysis element.
  ///
  /// @param[in] analysis  Analysis element node.
  void SetAnalysis(const xmlpp::Element* analysis);

  /// Extracts the approximation from the configurations.
  ///
  /// @param[in] approx  Approximation element node.
  void SetApproximation(const xmlpp::Element* approx);

  /// Extracts limits for analysis.
  ///
  /// @param[in] limits  An XML element containing various limits.
  void SetLimits(const xmlpp::Element* limits);

  /// Container for input files for analysis.
  /// These input files contain fault trees, events, etc.
  std::vector<std::string> input_files_;
  core::Settings settings_;  ///< Settings for specific analysis.
  std::string output_path_;  ///< The output destination.
};

}  // namespace scram

#endif  // SCRAM_SRC_CONFIG_H_
