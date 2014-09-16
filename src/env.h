/// @file env.h
/// Environmental Variables
#ifndef SCRAM_ENV_H_
#define SCRAM_ENV_H_

#include <string>

namespace scram {

/// @class Env
/// Provides environmental variables.
class Env {
 public:
  Env();

  /// @returns the location of the RelaxNG schema.
  std::string rng_schema();

 private:
  /// Installation directory.
  static std::string instdir_;
};

}  // namespace scram

#endif  // SCRAM_ENV_H_
