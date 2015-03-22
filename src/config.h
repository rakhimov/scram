/// @file config.h
/// Configuration management facilities to make various setups for analysis
/// possible.

#ifndef SCRAM_SRC_CONFIG_H_
#define SCRAM_SRC_CONFIG_H_

#include <string>
#include <vector>

#include "settings.h"

namespace scram {

class Config {
 public:
  /// A constructor with configurations for the analysis.
  /// Reads and validates the configurations.
  /// @param[in] config_file XML file with configurations.
  /// @throws ValidationError if the configurations have problems.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the file is not accessable.
  explicit Config(std::string config_file);

  /// @retuns input files for analysis.
  inline const std::vector<std::string>& input_files() const {
    return input_files_;
  }

  /// @returns the settings for analysis.
  inline const Settings& settings() const { return settings_; }

  /// @returns the output destination if any.
  inline std::string output_path() const { return output_path_; }

 private:
  /// Container for input files for analysis.
  /// These input files contain fault trees, events, etc.
  std::vector<std::string> input_files_;

  /// Settings for specific analysis.
  Settings settings_;

  /// The output destination.
  std::string output_path_;
};

}  // namespace scram

#endif  // SCRAM_SRC_CONFIG_H_
