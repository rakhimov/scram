/// @file logger.h
/// Logging capability for various purposes, such as warnings and debugging.
/// This logging facility caters mostly developers.
/// The design and code are inspired by the C++ logging framework of Petru
/// Marginean, published at http://www.drdobbs.com/cpp/logging-in-c/201804215
#ifndef SCRAM_SRC_LOGGER_H_
#define SCRAM_SRC_LOGGER_H_

#include <cstdio>
#include <iostream>

namespace scram {
/// @def LOG()
/// General logging for information purposes.
#define LOG()  if (INFO > scram::Logger::level()); \
  else scram::Logger().Get(INFO)

/// @def LOG(level)
/// Logging with the level defined.
#define LLOG(level) if (level > scram::Logger::level()); \
  else scram::Logger().Get(level)

/// @enum LogLevel
/// Levels for log statements.
enum LogLevel {  // The numbers are used for array indices.
  ERROR = 0,  ///< Noncritical errors only.
  WARNING,  ///< Warnings for users, such as assumptions and corrections.
  INFO,  ///< Information for users, such as running time and complexity.
  // Debugging logs are structured depending on the code under execution.
  // The deeper the code is located, the higher the level should be.
  // For example, if class A uses some utility class B, class B should have
  // higher debugging levels. This is only a recommendation.
  DEBUG1,  ///< Minimal debugging information.
  DEBUG2,  ///< Debugging information for the code inside of DEBUG1.
  DEBUG3,  ///< Debugging information for the code inside of DEBUG2.
  DEBUG4,  ///< Debugging information for the code inside of DEBUG3.
  DEBUG5  ///< Debugging information for the code inside of DEBUG4.
};

/// @class Logger
/// This is a general purpose logger; however, its main usage is asserted to be
/// for debugging. All messages are directed to the standard error in a
/// thread-safe way. This class may be expanded and modified in future to
/// include more levels, prefixes, and logging types if it deems necessary.
///
/// @warning Do not place any state-changing expressions with the LOG
///          macro as they may not run if the report level excludes the
///          specified level.
///
/// @warning Do not place leading spaces, newline, or tabs in messages because
///          it will mess up the level-dependent printing.
class Logger {
 public:
  Logger() {}

  /// Flashes all the logs into the standard error upon destruction.
  ~Logger() {
    os_ << std::endl;
    // fprintf used for thread safety.
    fprintf(stderr, "%s", os_.str().c_str());
    fflush(stderr);
  }

  /// This function can be to get and set the cut-off level for logging.
  /// @returns The cut-off level for reporting.
  static LogLevel& level();

  /// Returns a string stream by reference that is flushed to stderr by
  /// the Logger class destructor.
  /// @param[in] level The log level for the information.
  inline std::ostringstream& Get(LogLevel level) {
    os_ << Logger::level_to_string_[level] << ": ";
    os_ << std::string(level >= DEBUG1 ? 0 : level - DEBUG1 + 1, '\t');
    return os_;
  }

 private:
  /// Restrict copy construction.
  Logger(const Logger&);

  /// Restrict copy assignment.
  Logger& operator=(const Logger&);

  /// Main stringstream to gather the logs.
  std::ostringstream os_;

  /// Cut-off log level for the entire logging session.
  static LogLevel level_;

  /// Translates the logging level into a string. The index is the value
  /// of the enum.
  static const char* level_to_string_[];
};

const char* Logger::level_to_string_[] = {"ERROR", "WARNING", "INFO", "DEBUG1",
                                          "DEBUG2", "DEBUG3", "DEBUG4",
                                          "DEBUG5"};
LogLevel Logger::level_ = ERROR;

LogLevel& Logger::level() { return level_; }

}  // namespace scram

#endif  // SCRAM_SRC_LOGGER_H_
