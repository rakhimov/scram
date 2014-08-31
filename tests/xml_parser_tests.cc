#include "xml_parser_tests.h"

#include <iostream>
#include <fstream>
#include <gtest/gtest.h>

#include "error.h"

#include "tools.h"

using namespace std;

void XMLParserTests::FillSnippet(std::stringstream& ss) {
  ss << "<" << outer_node_ << ">"
     << "<" << inner_node_ << ">" << inner_content_
     << "</" << inner_node_ << ">"
     << "</" << outer_node_ << ">";
}

void XMLParserTests::FillBadSnippet(std::stringstream& ss) {
  ss << "<" << outer_node_ << ">"
     << "</" << outer_node_ << ">";
}

void XMLParserTests::FillSchema(std::stringstream& ss) {
  ss << "<grammar xmlns=\"http://relaxng.org/ns/structure/1.0\"" << std::endl
     << "datatypeLibrary=\"http://www.w3.org/2001/XMLSchema-datatypes\">" << std::endl
     << "  " << "<start>" << std::endl
     << "  " << "<element name =\"" << outer_node_ << "\">" << std::endl
     << "  " << "  " << "<element name =\"" << inner_node_ << "\">" << std::endl
     << "  " << "    " << "<text/>" << std::endl
     << "  " << "  " << "</element>" << std::endl
     << "  " << "</element>" << std::endl
     << "  " << "</start>" << std::endl
     << "</grammar>";
}

void XMLParserTests::SetUp() {
  inner_node_ = "inside";
  outer_node_ = "outside";
  inner_content_ = "inside_content";
}

void XMLParserTests::TearDown() {}

TEST_F(XMLParserTests, WithoutSchema) {
  std::stringstream snippet("");
  FillSnippet(snippet);
  scram::XMLParser parser;
  EXPECT_NO_THROW(parser.Init(snippet));
}

TEST_F(XMLParserTests, WithSchema) {
  std::stringstream snippet("");
  FillSnippet(snippet);
  stringstream schema("");
  FillSchema(schema);
  scram::XMLParser parser;
  EXPECT_NO_THROW(parser.Init(snippet));
  EXPECT_NO_THROW(parser.Validate(schema));
}

TEST_F(XMLParserTests, WithError) {
  stringstream snippet("");
  FillBadSnippet(snippet);
  std::stringstream schema("");
  FillSchema(schema);
  scram::XMLParser parser;
  EXPECT_NO_THROW(parser.Init(snippet));
  EXPECT_THROW(parser.Validate(schema), scram::ValidationError);
}
