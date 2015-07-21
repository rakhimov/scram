/// @file xml_parser.h
/// XML Parser.
#ifndef SCRAM_SRC_XML_PARSER_H_
#define SCRAM_SRC_XML_PARSER_H_

#include <sstream>

#include <boost/shared_ptr.hpp>
#include <libxml++/libxml++.h>

namespace scram {

/// @class XMLParser
/// A helper class to hold XML file data and provide automatic validation.
class XMLParser {
 public:
  /// Initializes a parser with an XML snippet.
  ///
  /// @param[in] xml_input_snippet An XML snippet to be used as input.
  ///
  /// @throws ValidationError There are problems loading the XML snippet.
  explicit XMLParser(const std::stringstream& xml_input_snippet);

  /// Resets the parser.
  ~XMLParser();

  /// Validates the file against a schema.
  ///
  /// @param[in] xml_schema_snippet The schema to validate against.
  ///
  /// @throws ValidationError The XML file failed schema validation.
  /// @throws LogicError The schema could not be parsed.
  /// @throws Error Could not create validating context.
  void Validate(const std::stringstream& xml_schema_snippet);

  /// @returns The parser's document.
  inline const xmlpp::Document* Document() const {
    return parser_->get_document();
  }

 private:
  boost::shared_ptr<xmlpp::DomParser> parser_;  ///< File parser.
};

}  // namespace scram

#endif  // SCRAM_SRC_XML_PARSER_H_
