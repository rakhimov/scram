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

/// @file logger.h
/// Logging capability for various purposes,
/// such as warnings and debugging.
/// This logging facility caters mostly developers.
/// The design and code are inspired by
/// the C++ logging framework of Petru Marginean,
/// published at http://www.drdobbs.com/cpp/logging-in-c/201804215
///
/// The timing facilities are inspired by
/// the talk of Bryce Adelstein "Benchmarking C++ Code" at CppCon 2015.

#ifndef SCRAM_SRC_LOGGER_H_
#define SCRAM_SRC_LOGGER_H_

#include <cstdint>

#include <chrono>
#include <sstream>

#include <boost/noncopyable.hpp>
#include <boost/preprocessor/cat.hpp>

namespace scram {

/// Takes a current time stamp in nanoseconds.
#define TIME_STAMP() std::chrono::steady_clock::now().time_since_epoch().count()

/// Starts the timing in nanoseconds.
///
/// @param[out] var  A unique name for time variable in the scope.
#define CLOCK(var) std::uint64_t var = TIME_STAMP()

/// Calculates the time duration since the start of the clock in seconds.
///
/// @param[in] var  The variable initialized by the CLOCK macro (in the past!).
#define DUR(var) (TIME_STAMP() - var) * 1e-9

/// Creates an automatic unique logging timer for a scope.
#define TIMER(level, ...) \
  Timer<level> BOOST_PP_CAT(timer_, __LINE__)(__VA_ARGS__)

/// Logging with a level.
#define LOG(level) \
  if (level <= scram::Logger::report_level()) scram::Logger().Get(level)

/// Conditional logging with a level.
#define BLOG(level, cond) \
  if (cond) LOG(level)

/// Levels for log statements.
enum LogLevel {  // The numbers are used for array indices.
  ERROR = 0,  ///< Non-critical errors only.
  WARNING,  ///< Warnings for users, such as assumptions and corrections.
  INFO,  ///< Information for users, such as running time and complexity.
  // Debugging logs are structured depending on the code under execution.
  // The deeper the code is located, the higher the level should be.
  // For example, if class A uses some utility class B,
  // class B should have higher debugging levels.
  // This is only a recommendation.
  DEBUG1,  ///< Minimal debugging information.
  DEBUG2,  ///< Debugging information for the code inside of DEBUG1.
  DEBUG3,  ///< Debugging information for the code inside of DEBUG2.
  DEBUG4,  ///< Debugging information for the code inside of DEBUG3.
  DEBUG5  ///< Debugging information for the code inside of DEBUG4.
};

const int kMaxVerbosity = 7;  ///< The index of the last level.

/// This is a general purpose logger;
/// however, its main usage is asserted to be for debugging.
/// All messages are directed to the standard error in a thread-safe way.
/// This class may be expanded and modified in future
/// to include more levels, prefixes, and logging types
/// if it deems necessary.
///
/// @warning Do not place any state-changing expressions
///          with the LOG macro as they may not run
///          if the report level excludes the specified level.
///
/// @warning Do not place leading spaces, newline, or tabs in messages
///          because it will mess up the level-dependent printing.
class Logger : private boost::noncopyable {
 public:
  /// Flashes all the logs into the standard error upon destruction.
  ~Logger() noexcept;

  /// @returns Reference to the cut-off level for reporting.
  static LogLevel report_level() { return report_level_; }

  /// Sets the reporting level cut-off.
  ///
  /// @param[in] level  The maximum level of logging.
  static void report_level(LogLevel level) { report_level_ = level; }

  /// Sets the reporting level cut-off from an integer.
  ///
  /// @param[in] level  Integer representation of the log level.
  ///
  /// @throws InvalidArgument  The level is out of range.
  static void SetVerbosity(int level);

  /// Returns a string stream by reference
  /// that is flushed to stderr by the Logger class destructor.
  ///
  /// @param[in] level  The log level for the information.
  ///
  /// @returns Formatted output stringstream with the log level information.
  std::ostringstream& Get(LogLevel level);

 private:
  /// Translates the logging level into a string.
  /// The index is the value of the enum.
  static const char* const kLevelToString_[];

  static LogLevel report_level_;  ///< Cut-off log level for reporting.

  std::ostringstream os_;  ///< Main stringstream to gather the logs.
};

/// Automatic (scoped) timer to log process duration.
template <LogLevel Level>
class Timer {
 public:
  /// @param[in] process_name  The process being logged.
  explicit Timer(const char* process_name)
      : process_name_(process_name), process_time_(TIME_STAMP()) {
    LOG(Level) << process_name_ << "...";
  }

  /// Puts the accumulated time into the logs.
  ~Timer() {
    LOG(Level) << "Finished " << process_name_ << " in " << DUR(process_time_);
  }

 private:
  const char* process_name_;  ///< The process name to be logged.
  std::uint64_t process_time_;  ///< The process start time.
};

}  // namespace scram

#endif  // SCRAM_SRC_LOGGER_H_
