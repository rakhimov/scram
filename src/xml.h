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
/// Adaptors and helper functions provide read-only facilities.

#ifndef SCRAM_SRC_XML_H_
#define SCRAM_SRC_XML_H_

#include <memory>
#include <string>
#include <type_traits>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <libxml++/libxml++.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "error.h"

namespace scram {

namespace xml {

/// Returns XML line number message.
inline std::string GetLine(const xmlpp::Node* xml_node) {
  return "Line " + std::to_string(xml_node->get_line()) + ":\n";
}

/// Gets a number from an XML value.
///
/// @tparam T  Numeric type.
///
/// @param[in] value  The non-empty value string.
///
/// @returns The interpreted value.
///
/// @throws ValidationError  Casting is unsuccessful.
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, T>
CastValue(const std::string& value) {
  try {
    return boost::lexical_cast<T>(value);
  } catch (boost::bad_lexical_cast&) {
    throw ValidationError("Failed to interpret value '" + value +
                          "' to a number.");
  }
}

/// Specialization for Boolean values.
template <>
inline bool CastValue<bool>(const std::string& value) {
  if (value == "true" || value == "1")
    return true;
  if (value == "false" || value == "0")
    return false;
  throw LogicError("Boolean types must be validated in schema.");
}

/// XML Element adaptor.
class Element {
 public:
  /// @param[in] element  The element in the XML document.
  explicit Element(const xmlpp::Element* element) : element_(*element) {}

  /// @returns The name of the XML element.
  std::string name() const { return element_.get_name(); }

  /// Retrieves the XML element's attribute values.
  ///
  /// @param[in] name  The name of the requested attribute.
  ///
  /// @returns The attribute value or
  ///          empty string if no attribute (optional attribute).
  ///
  /// @pre XML attributes never contain empty strings.
  std::string attribute(const std::string& name) const {
    std::string value = element_.get_attribute_value(name);
    boost::trim(value);
    return value;
  }

  /// @returns The XML element's text.
  std::string text() const {
    std::string content = element_.get_child_text()->get_content();
    boost::trim(content);
    return content;
  }

  /// Generic attribute value extraction following XML data types.
  ///
  /// @tparam T  The attribute value type (numeric).
  ///
  /// @param[in] name  The name of the attribute.
  ///
  /// @returns The value of type T interpreted from attribute value.
  ///          None if the attribute doesn't exists (optional).
  ///
  /// @throws ValidationError  Casting is unsuccessful.
  template <typename T>
  std::enable_if_t<std::is_arithmetic<T>::value, boost::optional<T>>
  attribute(const std::string& name) const {
    std::string value = attribute(name);
    if (value.empty())
      return {};
    try {
      return CastValue<T>(value);
    } catch (ValidationError& err) {
      err.msg(GetLine(&element_) + "Attribute '" + name + "': " + err.msg());
      throw;
    }
  }

  /// Generic text value extraction following XML data types.
  ///
  /// @tparam T  The attribute value type (numeric).
  ///
  /// @returns The value of type T interpreted from attribute value.
  ///
  /// @pre The text is not empty.
  ///
  /// @throws ValidationError  Casting is unsuccessful.
  template <typename T>
  std::enable_if_t<std::is_arithmetic<T>::value, T> text() const {
    try {
      return CastValue<T>(text());
    } catch (ValidationError& err) {
      err.msg(GetLine(&element_) + "Text element: " + err.msg());
      throw;
    }
  }

 private:
  const xmlpp::Element& element_;  ///< The main data location.
};

}  // namespace xml

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
inline std::string GetAttributeValue(const xmlpp::Attribute* attribute) {
  std::string value = attribute->get_value();
  boost::trim(value);
  return value;
}
/// Convenience function to retrieve element optional attribute values.
inline std::string GetAttributeValue(const xmlpp::Element* element,
                                     const std::string& attribute_name) {
  const xmlpp::Attribute* attribute = element->get_attribute(attribute_name);
  return attribute ? GetAttributeValue(attribute) : "";
}

/// Returns XML line number message.
inline std::string GetLine(const xmlpp::Node* xml_node) {
  return "Line " + std::to_string(xml_node->get_line()) + ":\n";
}

/// Gets a number from an XML attribute.
///
/// @tparam T  Numerical type.
///
/// @param[in] attribute  The XML element attribute.
///
/// @returns The interpreted value.
///
/// @throws ValidationError  Casting is unsuccessful.
///                          The error message will include the line number.
template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type
CastAttributeValue(const xmlpp::Attribute* attribute) {
  try {
    return boost::lexical_cast<T>(GetAttributeValue(attribute));
  } catch (boost::bad_lexical_cast&) {
    throw ValidationError(GetLine(attribute) +
                          "Failed to interpret attribute '" +
                          attribute->get_name() + "' to a number.");
  }
}
/// Specialization for Boolean values in XML attributes.
template <>
inline bool CastAttributeValue<bool>(const xmlpp::Attribute* attribute) {
  std::string value = GetAttributeValue(attribute);
  if (value == "true" || value == "1")
    return true;
  if (value == "false" || value == "0")
    return false;
  throw LogicError("Boolean types must be validated in schema.");
}

/// Convenience overload to cast XML element attribute value by name.
template <typename T>
T CastAttributeValue(const xmlpp::Element* element,
                     const std::string& attribute) {
    return CastAttributeValue<T>(element->get_attribute(attribute));
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
