/// @file xml_parser.cc
/// Implementation of XML Parser.
#include "xml_parser.h"

#include <string>

#include "error.h"
#include "relax_ng_validator.h"

namespace scram {

XMLParser::XMLParser(const std::stringstream& xml_input_snippet) {
  parser_ = boost::shared_ptr<xmlpp::DomParser>(new xmlpp::DomParser());
  try {
    parser_->parse_memory(xml_input_snippet.str());
    assert(parser_->get_document());
  } catch (std::exception& ex) {
    throw ValidationError("Error loading XML file: " + std::string(ex.what()));
  }
}

XMLParser::~XMLParser() { parser_.reset(); }

void XMLParser::Validate(const std::stringstream& xml_schema_snippet) {
  RelaxNGValidator validator;
  validator.ParseMemory(xml_schema_snippet.str());
  validator.Validate(parser_->get_document());
}

}  // namespace scram
