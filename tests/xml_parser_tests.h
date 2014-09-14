#ifndef SCRAM_TESTS_XML_PARSER_TESTS_H_
#define SCRAM_TESTS_XML_PARSER_TESTS_H_

#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "xml_parser.h"

class XMLParserTests : public ::testing::Test {
 public:
  virtual void SetUp();
  virtual void TearDown();

 protected:
  void FillSnippet(std::stringstream &ss);
  void FillBadSnippet(std::stringstream &ss);
  void FillSchema(std::stringstream &ss);
  void FillBadSchema(std::stringstream &ss);
  std::string outer_node_;
  std::string inner_node_;
  std::string inner_content_;
};

#endif  // SCRAM_TESTS_XML_PARSER_TESTS_H_
