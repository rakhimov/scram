/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#ifndef SCRAM_TESTS_XML_PARSER_TESTS_H_
#define SCRAM_TESTS_XML_PARSER_TESTS_H_

#include "xml_parser.h"

#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace scram {
namespace test {

class XmlParserTests : public ::testing::Test {
 protected:
  void SetUp() override {
    inner_node_ = "inside";
    outer_node_ = "outside";
    inner_content_ = "inside_content";
  }

  void TearDown() override {}

  /// Fills stream with XML text.
  ///
  /// @param[out] ss  Empty stream.
  /// @param[in] malformed  Make the snippet illegal XML.
  void FillSnippet(std::stringstream& ss, bool malformed = false);

  /// Fills RleaxNG schema.
  ///
  /// @param[out] ss  Empty stream.
  /// @param[in] malformed  Make the schema illegal XML.
  void FillSchema(std::stringstream& ss, bool malformed = false);

 private:
  std::string outer_node_;  ///< Outermost element name.
  std::string inner_node_;  ///< Innermost element name.
  std::string inner_content_;  ///< Text content of the innermost element.
};

}  // namespace test
}  // namespace scram

#endif  // SCRAM_TESTS_XML_PARSER_TESTS_H_
