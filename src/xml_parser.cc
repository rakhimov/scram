/// @file xml_parser.cc
/// Implementation of XML Parser.
#include "xml_parser.h"

#include <stdlib.h>
#include <string>

#include "error.h"
#include "relax_ng_validator.h"

namespace scram {

XMLParser::~XMLParser() { parser_.reset(); }

void XMLParser::Init(const std::stringstream& xml_input_snippet) {
  parser_ = boost::shared_ptr<xmlpp::DomParser>(new xmlpp::DomParser());
  try {
    parser_->parse_memory(xml_input_snippet.str());
    if (!parser_) throw ValidationError("Could not parse XML file.");
  } catch (std::exception& ex) {
    throw ValidationError("Error loading XML file: " + std::string(ex.what()));
  }
}

void XMLParser::Validate(const std::stringstream& xml_schema_snippet) {
  RelaxNGValidator validator;
  validator.parse_memory(xml_schema_snippet.str());
  validator.Validate(this->Document());
}

const xmlpp::Document* XMLParser::Document() {
  const xmlpp::Document* doc = parser_->get_document();
  return doc;
}

}  // namespace scram
