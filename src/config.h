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
  /// @param[in] config_file XML file with configurations.
  /// @throws ValidationError if the configurations have problems.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the file is not accessible.
  explicit Config(std::string config_file);

  /// @returns input files for analysis.
  inline const std::vector<std::string>& input_files() const {
    return input_files_;
  }

  /// @returns the settings for analysis.
  inline const Settings& settings() const { return settings_; }

  /// @returns the output destination if any.
  inline std::string output_path() const { return output_path_; }

 private:
  /// Extracts analysis types to be performed from analysis element.
  /// @param[in] analysis Analysis element node.
  void SetAnalysis(const xmlpp::Element* analysis);

  /// Extracts approximations from the configurations.
  /// @param[in] approx Approximation element node.
  void SetApprox(const xmlpp::Element* approx);

  /// Extracts limits for analysis.
  /// @param[in] limits An XML element containing various limits.
  void SetLimits(const xmlpp::Element* limits);

  /// Interprets the given string into a boolean value.
  /// @param[in] flag A flag that can be 0, 1, true, or false.
  /// @returns The interpreted boolean.
  bool GetBoolFromString(std::string flag);

  /// Container for input files for analysis.
  /// These input files contain fault trees, events, etc.
  std::vector<std::string> input_files_;
  Settings settings_;  ///< Settings for specific analysis.
  std::string output_path_;  ///< The output destination.
};

}  // namespace scram

#endif  // SCRAM_SRC_CONFIG_H_
