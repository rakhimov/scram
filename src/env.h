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
  /// @returns the location of the RelaxNG schema.
  static const std::string rng_schema();

 private:
  /// Installation directory.
  static std::string instdir_;
};

}  // namespace scram

#endif  // SCRAM_SRC_ENV_H_
