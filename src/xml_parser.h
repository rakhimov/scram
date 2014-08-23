#ifndef SCRAM_XML_PARSER_H_
#define SCRAM_XML_PARSER_H_

#include <sstream>

#include <libxml++/libxml++.h>
#include <boost/shared_ptr.hpp>

namespace scram {

/// A helper class to hold xml file data and provide automatic
/// validation.
class XMLParser {
 public:
  /// Constructor.
  XMLParser();

  /// Destructor.
  ~XMLParser();

  /// Initializes a parser with an xml snippet.
  /// @param[in] input An xml snippet to be used as input.
  void Init(const std::stringstream& input);

  /// Validates the file agaisnt a schema.
  /// @param[in] schema The schema to validate agaisnt.
  void Validate(const std::stringstream& schema);

  /// @return The parser's document.
  xmlpp::Document* Document();

 private:
  /// File parser.
  boost::shared_ptr<xmlpp::DomParser> parser_;
};

}  // namespace scram

#endif  // SCRAM_XML_PARSER_H_
