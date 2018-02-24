/*
 * Copyright (C) 2018 Olzhas Rakhimov
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

#include "ext/version.h"

#include <catch.hpp>

namespace scram::test {

TEST_CASE("invalid version extraction", "[version]") {
  const char* versions[] = {
      "",           "string",      "1string",      "1.string",
      "0.1.string", "0.1.2string", "0.1.2.string", " 0.2.3",
      "1_61",       "1,61",        "1'61",         "1-61",
      "0.2.2.",     ".1",          "0..1",         "0.1..",
      "0.1.2a",     "-1"};

  for (auto& version : versions) {
    CAPTURE(version);
    CHECK(!ext::extract_version(version));
  }
}

TEST_CASE("valid default version extraction", "[version]") {
  std::pair<const char*, std::array<int, 3>> versions[] = {
      {"0", {}},
      {"0.1", {0, 1}},
      {"0.1.0", {0, 1}},
      {"0.1.9", {0, 1, 9}},
      {"5.1.9", {5, 1, 9}},
      {"999.9999.99999", {999, 9999, 99999}},
  };

  for (const auto& [version, expected] : versions) {
    CAPTURE(version);
    auto numbers = ext::extract_version(version);
    CHECK(numbers);
    if (numbers)
      CHECK(*numbers == expected);
  }
}

TEST_CASE("valid version extraction w/ custom separator", "[version]") {
  std::array<int, 3> expected = {0, 1, 2};
  std::pair<const char*, char> versions[] = {
      {"0.1.2", '.'}, {"0_1_2", '_'}, {"0-1-2", '-'}, {"0'1'2", '\''},
      {"0 1 2", ' '}, {"05152", '5'}, {"0s1s2", 's'}, {"0\n1\n2", '\n'},
  };

  for (const auto& [version, separator] : versions) {
    CAPTURE(version);
    auto numbers = ext::extract_version(version, separator);
    CHECK(numbers);
    if (numbers)
      CHECK(*numbers == expected);
  }
}

TEST_CASE("valid version from substring", "[version]") {
  std::array<int, 3> expected = {0, 1, 2};
  std::string_view version = "0.1.2-alpha";
  CAPTURE(version);
  auto numbers = ext::extract_version(version.substr(0, 5));
  if (numbers)
    CHECK(*numbers == expected);
}

}  // namespace scram::test
