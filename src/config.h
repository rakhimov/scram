/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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
/// Configuration management facilities to make various setups for analysis
/// possible.

#ifndef SCRAM_SRC_CONFIG_H_
#define SCRAM_SRC_CONFIG_H_

#include <string>
#include <vector>

#include <libxml++/libxml++.h>

#include "settings.h"

namespace scram {

/// @class Config
/// This class processes configuration files for analysis.
/// The class contains all the setup and state to initialize general
/// analysis.
class Config {
 public:
  /// A constructor with configurations for analysis.
  /// Reads and validates the configurations.
  ///
  /// @param[in] config_file XML file with configurations.
  ///
  /// @throws ValidationError The configurations have problems.
  /// @throws ValueError Input values are not valid.
  /// @throws IOError The file is not accessible.
  explicit Config(const std::string& config_file);

  /// @returns input files for analysis.
  inline const std::vector<std::string>& input_files() const {
    return input_files_;
  }

  /// @returns the settings for analysis.
  inline const Settings& settings() const { return settings_; }

  /// @returns the output destination if any.
  inline const std::string& output_path() const { return output_path_; }

 private:
  /// Extracts analysis types to be performed from analysis element.
  ///
  /// @param[in] analysis Analysis element node.
  void SetAnalysis(const xmlpp::Element* analysis);

  /// Extracts approximations from the configurations.
  ///
  /// @param[in] approx Approximation element node.
  void SetApprox(const xmlpp::Element* approx);

  /// Extracts limits for analysis.
  ///
  /// @param[in] limits An XML element containing various limits.
  void SetLimits(const xmlpp::Element* limits);

  /// Interprets the given string into a boolean value.
  ///
  /// @param[in] flag A flag that can be 0, 1, true, or false.
  ///
  /// @returns The interpreted boolean.
  bool GetBoolFromString(const std::string& flag);

  /// Container for input files for analysis.
  /// These input files contain fault trees, events, etc.
  std::vector<std::string> input_files_;
  Settings settings_;  ///< Settings for specific analysis.
  std::string output_path_;  ///< The output destination.
};

}  // namespace scram

#endif  // SCRAM_SRC_CONFIG_H_
