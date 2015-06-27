/// @file env.h
/// Environmental Variables
#ifndef SCRAM_SRC_ENV_H_
#define SCRAM_SRC_ENV_H_

#include <string>

namespace scram {

/// @class Env
/// Provides environmental variables.
class Env {
 public:
  /// @returns the location of the RelaxNG schema for configuration files.
  static std::string config_schema();

  /// @returns the location of the RelaxNG schema for input files.
  static std::string input_schema();

  /// @returns the location of the RelaxNG schema for output report files.
  static std::string report_schema();

 private:
  static const std::string instdir_;  ///< Installation directory.
};

}  // namespace scram

#endif  // SCRAM_SRC_ENV_H_
