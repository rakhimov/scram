/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file xml.h
/// XML helper facilities to work with libxml++.

#ifndef SCRAM_SRC_XML_H_
#define SCRAM_SRC_XML_H_

#include <memory>
#include <string>
#include <type_traits>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "error.h"

namespace scram {

/// Initializes a DOM parser
/// and converts library exceptions into local errors.
///
/// All XInclude directives are processed into the final document.
///
/// @param[in] file_path  Path to the xml file.
///
/// @returns A parser with a well-formed, initialized document.
///
/// @throws ValidationError  There are problems loading the XML file.
inline std::unique_ptr<xmlpp::DomParser> ConstructDomParser(
    const std::string& file_path) {
  try {
    auto dom_parser = std::make_unique<xmlpp::DomParser>(file_path);
    xmlXIncludeProcessFlags(dom_parser->get_document()->cobj(),
                            XML_PARSE_NOBASEFIX);
    dom_parser->get_document()->process_xinclude();
    return dom_parser;
  } catch (const xmlpp::exception& ex) {
    throw ValidationError("XML file is invalid:\n" + std::string(ex.what()));
  }
}

/// Helper function to statically cast to XML element.
///
/// @param[in] node  XML node known to be XML element.
///
/// @returns XML element cast from the XML node.
///
/// @warning The node must be an XML element.
inline const xmlpp::Element* XmlElement(const xmlpp::Node* node) {
  return static_cast<const xmlpp::Element*>(node);
}

/// Returns Normalized (trimmed) string value of an XML element attribute.
inline std::string GetAttributeValue(const xmlpp::Element* element,
                                     const std::string& attribute_name) {
  std::string value = element->get_attribute_value(attribute_name);
  boost::trim(value);
  return value;
}

/// Returns XML line number message.
inline std::string GetLine(const xmlpp::Node* xml_node) {
  return "Line " + std::to_string(xml_node->get_line()) + ":\n";
}

/// Gets a number from an XML attribute.
///
/// @tparam T  Numerical type.
///
/// @param[in] element  XML element with the attribute.
/// @param[in] attribute  The name of the attribute.
///
/// @returns The interpreted value.
///
/// @throws ValidationError  Casting is unsuccessful.
///                          The error message will include the line number.
template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type
CastAttributeValue(const xmlpp::Element* element,
                   const std::string& attribute) {
  try {
    return boost::lexical_cast<T>(GetAttributeValue(element, attribute));
  } catch (boost::bad_lexical_cast&) {
    throw ValidationError(GetLine(element) + "Failed to interpret attribute '" +
                          attribute + "' to a number.");
  }
}

/// Returns Normalized content of an XML text node.
inline std::string GetContent(const xmlpp::TextNode* child_text) {
  std::string content = child_text->get_content();
  boost::trim(content);
  return content;
}

/// Gets a number from an XML text.
///
/// @tparam T  Numerical type.
///
/// @param[in] element  XML element with the text.
///
/// @returns The interpreted value.
///
/// @throws ValidationError  Casting is unsuccessful.
///                          The error message will include the line number.
template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type
CastChildText(const xmlpp::Element* element) {
  std::string content = GetContent(element->get_child_text());
  try {
    return boost::lexical_cast<T>(content);
  } catch (boost::bad_lexical_cast&) {
    throw ValidationError(GetLine(element) + "Failed to interpret text '" +
                          content + "' to a number.");
  }
}

}  // namespace scram

#endif  // SCRAM_SRC_XML_H_
