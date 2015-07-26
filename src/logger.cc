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
/// @file logger.cc
/// Initializing static members of Logger class.
#include "logger.h"

namespace scram {

const char* const Logger::kLevelToString_[] = {"ERROR", "WARNING", "INFO",
                                               "DEBUG1", "DEBUG2", "DEBUG3",
                                               "DEBUG4", "DEBUG5"};
LogLevel Logger::report_level_ = ERROR;

LogLevel& Logger::ReportLevel() { return report_level_; }

}  // namespace scram
