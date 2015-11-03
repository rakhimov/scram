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

#include "xml_parser_tests.h"

#include <iostream>

#include <gtest/gtest.h>

#include "error.h"
#include "relax_ng_validator.h"

namespace scram {
namespace test {

void XmlParserTests::FillSnippet(std::stringstream& ss) {
  ss << "<" << outer_node_ << ">"
     << "<" << inner_node_ << ">" << inner_content_
     << "</" << inner_node_ << ">"
     << "</" << outer_node_ << ">";
}

void XmlParserTests::FillBadSnippet(std::stringstream& ss) {
  ss << "<" << outer_node_ << ">"
     << "</" << outer_node_ << ">";
}

void XmlParserTests::FillSchema(std::stringstream& ss) {
  ss << "<grammar xmlns=\"http://relaxng.org/ns/structure/1.0\"" << std::endl
     << "datatypeLibrary=\"http://www.w3.org/2001/XMLSchema-datatypes\">"
     << std::endl
     << "  " << "<start>" << std::endl
     << "  " << "<element name =\"" << outer_node_ << "\">" << std::endl
     << "  " << "  " << "<element name =\"" << inner_node_ << "\">" << std::endl
     << "  " << "    " << "<text/>" << std::endl
     << "  " << "  " << "</element>" << std::endl
     << "  " << "</element>" << std::endl
     << "  " << "</start>" << std::endl
     << "</grammar>";
}

void XmlParserTests::FillBadSchema(std::stringstream& ss) {
  ss << "<grammar xmlns=\"http://relaxng.org/ns/structure/1.0\"" << std::endl
     << "datatypeLibrary=\"http://www.w3.org/2001/XMLSchema-datatypes\">"
     << std::endl
     << "  " << "<start>" << std::endl
     << "  " << "<element naem =\"" << outer_node_ << "\">" << std::endl
     << "  " << "  " << "<element name =\"" << inner_node_ << "\">" << std::endl
     << "  " << "    " << "<text/>" << std::endl
     << "  " << "  " << "</element>" << std::endl
     << "  " << "</element>" << std::endl
     << "  " << "</start>" << std::endl
     << "</grammar>";
}

void XmlParserTests::SetUp() {
  inner_node_ = "inside";
  outer_node_ = "outside";
  inner_content_ = "inside_content";
}

void XmlParserTests::TearDown() {}

// This is an indirect test of the validator.
TEST_F(XmlParserTests, RelaxNGValidator) {
  std::stringstream snippet("");
  FillSnippet(snippet);
  std::stringstream schema("");
  FillSchema(schema);

  XmlParserPtr parser;
  EXPECT_NO_THROW(parser = XmlParserPtr(new XmlParser(snippet)));

  RelaxNGValidator validator;
  const xmlpp::Document* doc = nullptr;
  EXPECT_NO_THROW(validator.ParseMemory(schema.str()));
  EXPECT_THROW(validator.Validate(doc), InvalidArgument);

  doc = parser->Document();
  validator = RelaxNGValidator();
  EXPECT_THROW(validator.Validate(doc), LogicError);  // No schema initialized.

  EXPECT_NO_THROW(validator.ParseMemory(schema.str()));
  EXPECT_NO_THROW(validator.Validate(doc));  // Initialized.
}

TEST_F(XmlParserTests, WithoutSchema) {
  std::stringstream snippet("");
  FillSnippet(snippet);
  EXPECT_NO_THROW(XmlParserPtr(new XmlParser(snippet)));
}

TEST_F(XmlParserTests, WithSchema) {
  std::stringstream snippet("");
  FillSnippet(snippet);
  std::stringstream schema("");
  FillSchema(schema);
  XmlParserPtr parser;
  EXPECT_NO_THROW(parser = XmlParserPtr(new XmlParser(snippet)));
  EXPECT_NO_THROW(parser->Validate(schema));
}

TEST_F(XmlParserTests, WithBadSchema) {
  std::stringstream snippet("");
  FillSnippet(snippet);
  std::stringstream schema("");
  FillBadSchema(schema);
  XmlParserPtr parser;
  EXPECT_NO_THROW(parser = XmlParserPtr(new XmlParser(snippet)));
  EXPECT_THROW(parser->Validate(schema), LogicError);
}

TEST_F(XmlParserTests, WithError) {
  std::stringstream snippet("");
  FillBadSnippet(snippet);
  std::stringstream schema("");
  FillSchema(schema);
  XmlParserPtr parser;
  EXPECT_NO_THROW(parser = XmlParserPtr(new XmlParser(snippet)));
  EXPECT_THROW(parser->Validate(schema), ValidationError);
}

}  // namespace test
}  // namespace scram
