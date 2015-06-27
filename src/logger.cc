/// @file logger.cc
/// Initializing static members of Logger class.
#include "logger.h"

namespace scram {

const char* const Logger::level_to_string_[] = {"ERROR", "WARNING", "INFO",
                                                "DEBUG1", "DEBUG2", "DEBUG3",
                                                "DEBUG4", "DEBUG5"};
LogLevel Logger::report_level_ = ERROR;

LogLevel& Logger::ReportLevel() { return report_level_; }

}  // namespace scram
