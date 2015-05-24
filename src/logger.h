/// @file logger.h
/// Logging capability for various purposes, such as warnings and debugging.
/// This logging caters mostly developers.
#ifndef SCRAM_SRC_LOGGER_H_
#define SCRAM_SRC_LOGGER_H_

#include <cstdio>
#include <iostream>
#include <sstream>

namespace scram {

/// @def LOG()
/// General logging without any discrimination against levels or types.
#define LOG() if (scram::Logger::active()) scram::Logger().Get()

/// @class Logger
/// This is a general purpose logger mainly for debugging purposes.
/// This class may be expanded and modified in future to include levels and
/// logging types if it deemed necessary.
class Logger {
 public:
  Logger() {}

  /// Flashes all the logs into standard output upon destruction.
  virtual ~Logger() {
    os << std::endl;
    // fprintf used for thread safety.
    fprintf(stdout, "%s", os.str().c_str());
    fflush(stdout);
  }

  /// Turns on or off the logging.
  static bool& active() { return active_; }

  /// Returns a string stream by reference that is flushed to stdout by
  /// the Logger class destructor.
  inline std::ostringstream& Get() { return os; }

 protected:
  /// Main stringstream to gather the logs.
  std::ostringstream os;

 private:
  /// Indication if logging should be collected.
  static bool active_;

  /// Restrict copy construction.
  Logger(const Logger&);

  /// Restrict copy assignment.
  Logger& operator=(const Logger&);
};

bool Logger::active_ = false;  // Off by default.

}  // namespace scram

#endif  // SCRAM_SRC_LOGGER_H_
