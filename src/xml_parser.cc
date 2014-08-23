#include "xml_parser.h"

#include <stdlib.h>
#include <string>

#include "error.h"
#include "relax_ng_validator.h"

namespace scram {

XMLParser::XMLParser() {}

XMLParser::~XMLParser() {
  parser_.reset();
}

void XMLParser::Init(const std::stringstream& xml_input_snippet) {
  parser_ = boost::shared_ptr<xmlpp::DomParser>(new xmlpp::DomParser());
  try {
    parser_->parse_memory(xml_input_snippet.str());
    if (!parser_) {
      throw ValidationError("Could not parse xml file.");
    }
  } catch (std::exception& ex) {
    throw ValidationError("Error loading xml file: " + std::string(ex.what()));
  }
}

void XMLParser::Validate(const std::stringstream& xml_schema_snippet) {
  RelaxNGValidator validator;
  validator.parse_memory(xml_schema_snippet.str());
  validator.Validate(this->Document());
}

xmlpp::Document* XMLParser::Document() {
  xmlpp::Document* doc = parser_->get_document();

  // This adds the capability to have nice include semantics.
  // This is introduced in libxml++2.6 2.36.
  // Not available on Ubuntu 12.04, so this commented out.
  // doc->process_xinclude();

  // This removes the stupid xml:base attribute that including adds,
  // but which is unvalidatable. The web is truly cobbled together
  // by a race of evil gnomes.
  xmlpp::Element* root = doc->get_root_node();
  xmlpp::NodeSet have_base = root->find("//*[@xml:base]");
  xmlpp::NodeSet::iterator it = have_base.begin();
  for (; it != have_base.end(); ++it) {
    reinterpret_cast<xmlpp::Element*>(*it)->remove_attribute("base", "xml");
  }
  return doc;
}

}  // namespace scram
