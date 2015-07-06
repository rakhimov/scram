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
  /// Resets the parser.
  ~XMLParser();

  /// Initializes a parser with an XML snippet.
  /// @param[in] input An XML snippet to be used as input.
  void Init(const std::stringstream& input);

  /// Validates the file against a schema.
  /// @param[in] schema The schema to validate against.
  void Validate(const std::stringstream& schema);

  /// @returns The parser's document.
  const xmlpp::Document* Document();

 private:
  boost::shared_ptr<xmlpp::DomParser> parser_;  ///< File parser.
};

}  // namespace scram

#endif  // SCRAM_SRC_XML_PARSER_H_
