/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

/// @file
/// Test helper functions.

#ifndef SCRAM_TESTS_UTILITY_H_
#define SCRAM_TESTS_UTILITY_H_

#include <string>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace scram {
namespace utility {

/// Generate unique file path for temporary files.
inline fs::path GenerateFilePath(const std::string& prefix = "scram_test") {
  fs::path unique_name = prefix + "-" + fs::unique_path().string();
  return fs::temp_directory_path() / unique_name;
}

}  // namespace utility
}  // namespace scram

#endif  // SCRAM_TESTS_UTILITY_H_
