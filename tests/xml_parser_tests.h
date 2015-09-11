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

#ifndef SCRAM_TESTS_XML_PARSER_TESTS_H_
#define SCRAM_TESTS_XML_PARSER_TESTS_H_

#include "xml_parser.h"

#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace scram {
namespace test {

class XMLParserTests : public ::testing::Test {
 public:
  virtual void SetUp();
  virtual void TearDown();

 protected:
  using XMLParserPtr = std::unique_ptr<XMLParser>;

  void FillSnippet(std::stringstream& ss);
  void FillBadSnippet(std::stringstream& ss);
  void FillSchema(std::stringstream& ss);
  void FillBadSchema(std::stringstream& ss);
  std::string outer_node_;
  std::string inner_node_;
  std::string inner_content_;
};

}  // namespace test
}  // namespace scram

#endif  // SCRAM_TESTS_XML_PARSER_TESTS_H_
