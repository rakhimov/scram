/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// XML helper facilities to work with libxml2.
/// Adaptors and helper functions provide read-only facilities.
///
/// @note All strings and characters are UTF-8 unless otherwise documented.
///
/// @note The facilities are designed specifically for SCRAM use cases.
///       The XML assumed to be well formed and simple.
///
/// @note libxml2 older versions are not const correct in API.
///
/// @warning Complex XML features are not handled or expected,
///          for example, DTD, namespaces, entries.

#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <libxml/parser.h>
#include <libxml/relaxng.h>
#include <libxml/tree.h>

#include "error.h"

namespace scram::xml {

namespace detail {  // Internal XML helper functions.

/// Gets a number from an XML value.
///
/// @tparam T  Numeric type.
///
/// @param[in] value  The non-empty value string.
///
/// @returns The interpreted value.
///
/// @throws ValidityError  The interpretation is unsuccessful.
template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, T> to(const std::string_view& value) {
  if constexpr (std::is_same_v<T, int>) {
    char* end_char = nullptr;
    std::int64_t ret = std::strtoll(value.data(), &end_char, 10);
    int len = end_char - value.data();
    if (len != value.size() || ret > std::numeric_limits<int>::max() ||
        ret < std::numeric_limits<int>::min()) {
      SCRAM_THROW(ValidityError("Failed to interpret value to int"))
          << errinfo_value(std::string(value));
    }
    return ret;

  } else if constexpr (std::is_same_v<T, double>) {  // NOLINT
    char* end_char = nullptr;
    double ret = std::strtod(value.data(), &end_char);
    int len = end_char - value.data();
    if (len != value.size() || ret == HUGE_VAL || ret == -HUGE_VAL) {
      SCRAM_THROW(ValidityError("Failed to interpret value to double"))
          << errinfo_value(std::string(value));
    }
    return ret;

  } else {
    static_assert(std::is_same_v<T, bool>, "Only default numeric types.");

    if (value == "true" || value == "1")
      return true;
    if (value == "false" || value == "0")
      return false;
    SCRAM_THROW(ValidityError("Failed to interpret value to bool"))
        << errinfo_value(std::string(value));
  }
}

/// Reinterprets the XML library UTF-8 string into C string.
///
/// @param[in] xml_string  The string provided by the XML library.
///
/// @returns The same string adapted for use as C string.
inline const char* from_utf8(const xmlChar* xml_string) noexcept {
  assert(xml_string);
  return reinterpret_cast<const char*>(xml_string);
}

/// Reinterprets C string as XML library UTF-8 string.
///
/// @param[in] c_string  The C byte-array encoding the XML string.
///
/// @returns The same string adapted for use in XML library functions.
///
/// @pre The C string has UTF-8 encoding.
inline const xmlChar* to_utf8(const char* c_string) noexcept {
  assert(c_string);
  return reinterpret_cast<const xmlChar*>(c_string);
}

/// Removes leading and trailing space characters from XML value string.
///
/// @param[in] text  The text in XML attribute or text nodes.
///
/// @returns View to the trimmed substring.
///
/// @pre The string is normalized by the XML parser.
inline std::string_view trim(const std::string_view& text) noexcept {
  auto pos_first = text.find_first_not_of(' ');
  if (pos_first == std::string_view::npos)
    return {};

  auto pos_last = text.find_last_not_of(' ');
  auto len = pos_last - pos_first + 1;

  return std::string_view(text.data() + pos_first, len);
}

/// Gets the last XML error converted from the library error codes.
///
/// @tparam T  The SCRAM error type to convert XML error into.
///
/// @param[in] xml_error  The error to translate.
///                       nullptr to retrieve the latest error in the library.
///
/// @returns The exception object to be thrown.
template <typename T>
T GetError(xmlErrorPtr xml_error = nullptr) {
  if (!xml_error)
    xml_error = xmlGetLastError();
  assert(xml_error && "No XML error is available.");
  T throw_error(xml_error->message);
  if (xml_error->file)
    throw_error << boost::errinfo_file_name(xml_error->file);
  if (xml_error->line)
    throw_error << boost::errinfo_at_line(xml_error->line);
  return throw_error;
}

}  // namespace detail

/// XML Element adaptor.
class Element {
 public:
  /// The range for elements.
  /// This is a simple view adaptor
  /// to the linked list XML elements.
  class Range {
   public:
    using value_type = Element;  ///< Minimal container for Element type.

    /// Iterator over range elements.
    class iterator
        : public boost::iterator_facade<iterator, Element,
                                        std::forward_iterator_tag, Element> {
      friend class boost::iterator_core_access;

     public:
      /// @param[in] element  The starting element in the list.
      ///                     nullptr signifies the end.
      explicit iterator(const xmlElement* element = nullptr)
          : element_(element) {}

     private:
      /// Standard iterator functionality required by the facade facilities.
      /// @{
      void increment() {
        assert(element_ && "Incrementing end iterator!");
        element_ = Range::findElement(element_->next);
      }
      bool equal(const iterator& other) const {
        return element_ == other.element_;
      }
      Element dereference() const { return Element(element_); }
      /// @}

      const xmlElement* element_;  ///< The current element.
    };

    using const_iterator = iterator;  ///< The container is immutable.

    /// Constructs the range for the intrusive list of XML Element nodes.
    ///
    /// @param[in] head  The head node of the list (may be non-Element node!).
    ///                  nullptr if the list is empty.
    explicit Range(const xmlNode* head) : begin_(findElement(head)) {}

    /// The range begin and end iterators.
    /// @{
    iterator begin() const { return begin_; }
    iterator end() const { return iterator(); }
    iterator cbegin() const { return begin_; }
    iterator cend() const { return iterator(); }
    /// @}

    /// @return true if the range contains no elements.
    bool empty() const { return begin() == end(); }

    /// @returns The number of Elements in the list.
    ///
    /// @note O(N) complexity.
    std::size_t size() const { return std::distance(begin(), end()); }

   private:
    /// Finds the first Element node in the list.
    ///
    /// @param[in] node  The starting node.
    ///                  nullptr for the end node.
    ///
    /// @returns The first Element type node.
    ///          nullptr if the list does not contain any Element nodes.
    static const xmlElement* findElement(const xmlNode* node) noexcept {
      while (node && node->type != XML_ELEMENT_NODE)
        node = node->next;
      return reinterpret_cast<const xmlElement*>(node);
    }

    iterator begin_;  ///< The first node with XML Element.
  };

  /// @param[in] element  The element in the XML document.
  explicit Element(const xmlElement* element) : element_(element) {
    assert(element_);
  }

  /// @returns The URI of the file containing the element.
  ///
  /// @pre The document has been loaded from a file.
  const char* filename() const { return detail::from_utf8(element_->doc->URL); }

  /// @returns The line number of the element.
  int line() const { return XML_GET_LINE(to_node()); }

  /// @returns The name of the XML element.
  ///
  /// @pre The element has a name.
  std::string_view name() const { return detail::from_utf8(element_->name); }

  /// Queries element attribute existence.
  ///
  /// @param[in] name  The non-empty attribute name.
  ///
  /// @returns true if the element has an attribute with the given name.
  ///
  /// @note This is an inefficient way to work with optional attributes.
  ///       Use the ``attribute(name)`` member function directly for optionals.
  bool has_attribute(const char* name) const {
    return xmlHasProp(to_node(), detail::to_utf8(name)) != nullptr;
  }

  /// Retrieves the XML element's attribute values.
  ///
  /// @param[in] name  The name of the requested attribute.
  ///
  /// @returns The attribute value or
  ///          empty string if no attribute (optional attribute).
  ///
  /// @pre XML attributes never contain empty strings.
  /// @pre XML attribute values are simple texts w/o DTD processing.
  std::string_view attribute(const char* name) const {
    const xmlAttr* property = xmlHasProp(to_node(), detail::to_utf8(name));
    if (!property)
      return {};
    const xmlNode* text_node = property->children;
    assert(text_node && text_node->type == XML_TEXT_NODE);
    assert(text_node->content);
    return detail::trim(detail::from_utf8(text_node->content));
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
  /// @throws ValidityError  Casting is unsuccessful.
  template <typename T>
  std::enable_if_t<std::is_arithmetic_v<T>, std::optional<T>>
  attribute(const char* name) const {
    std::string_view value = attribute(name);
    if (value.empty())
      return {};
    try {
      return detail::to<T>(value);
    } catch (ValidityError& err) {
      err << errinfo_element(std::string(Element::name()))
          << errinfo_attribute(name) << boost::errinfo_at_line(line())
          << boost::errinfo_file_name(filename());
      throw;
    }
  }

  /// @returns The XML element's text.
  ///
  /// @pre The Element has text.
  std::string_view text() const {
    const xmlNode* text_node = element_->children;
    while (text_node && text_node->type != XML_TEXT_NODE)
      text_node = text_node->next;
    assert(text_node && "Element does not have text.");
    assert(text_node->content && "Missing text in Element.");
    return detail::trim(detail::from_utf8(text_node->content));
  }

  /// Generic text value extraction following XML data types.
  ///
  /// @tparam T  The attribute value type (numeric).
  ///
  /// @returns The value of type T interpreted from attribute value.
  ///
  /// @pre The text is not empty.
  ///
  /// @throws ValidityError  Casting is unsuccessful.
  template <typename T>
  std::enable_if_t<std::is_arithmetic_v<T>, T> text() const {
    try {
      return detail::to<T>(text());
    } catch (ValidityError& err) {
      err << errinfo_element(std::string(name()))
          << boost::errinfo_at_line(line())
          << boost::errinfo_file_name(filename());
      throw;
    }
  }

  /// @param[in] name  The name of the child element.
  ///                  Empty string to request any first child element.
  ///
  /// @returns The first child element (with the given name).
  std::optional<Element> child(std::string_view name = "") const {
    for (Element element : children()) {
      if (name.empty() || name == element.name())
        return element;
    }
    return {};
  }

  /// @returns All the Element children.
  Range children() const { return Range(element_->children); }

  /// @param[in] name  The name to filter children elements.
  ///
  /// @returns The range of Element children with the given name.
  ///
  /// @pre The name must live at least as long as the returned range lives.
  auto children(std::string_view name) const {
    return children() |
           boost::adaptors::filtered([name](const Element& element) {
             return element.name() == name;
           });
  }

 private:
  /// Converts the data to its base.
  xmlNode* to_node() const {
    return reinterpret_cast<xmlNode*>(const_cast<xmlElement*>(element_));
  }

  const xmlElement* element_;  ///< The main data location.
};

/// The parser options passed to the library parser.
const int kParserOptions = XML_PARSE_XINCLUDE | XML_PARSE_NOBASEFIX |
                           XML_PARSE_NONET | XML_PARSE_NOXINCNODE |
                           XML_PARSE_COMPACT | XML_PARSE_HUGE;

class Validator;  // Forward declaration for validation upon DOM constructions.

/// XML DOM tree document.
class Document {
 public:
  /// Parses XML input document.
  /// All XInclude directives are processed into the final document.
  ///
  /// @param[in] file_path  The path to the document file.
  /// @param[in] validator  Optional validator against the RNG schema.
  ///
  /// @returns The initialized document.
  ///
  /// @throws IOError  The file is not available.
  /// @throws ParseError  There are XML parsing failures.
  /// @throws XIncludeError  XInclude resolution has failed.
  /// @throws ValidityError  The XML file is not valid.
  explicit Document(const std::string& file_path,
                    Validator* validator = nullptr);

  /// @returns The root element of the document.
  ///
  /// @pre The document has a root node.
  Element root() const {
    return Element(
        reinterpret_cast<const xmlElement*>(xmlDocGetRootElement(doc_.get())));
  }

  /// @returns The underlying data document.
  /// @{
  const xmlDoc* get() const { return doc_.get(); }
  xmlDoc* get() { return doc_.get(); }
  /// @}

 private:
  std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> doc_;  ///< The DOM document.
};

/// RelaxNG validator.
class Validator {
 public:
  /// @param[in] rng_file  The path to the schema file.
  ///
  /// @throws ParseError  RNG file parsing has failed.
  /// @throws LogicError  The XML library functions have failed internally.
  explicit Validator(const std::string& rng_file);

  /// Validates XML DOM documents against the schema.
  ///
  /// @param[in] doc  The initialized XML DOM document.
  ///
  /// @throws ValidityError  The document failed schema validation.
  void validate(const Document& doc) {
    xmlResetLastError();
    int ret = xmlRelaxNGValidateDoc(valid_ctxt_.get(),
                                    const_cast<xmlDoc*>(doc.get()));
    if (ret != 0)
      SCRAM_THROW(detail::GetError<ValidityError>());
  }

 private:
  /// The schema used by the validation context.
  std::unique_ptr<xmlRelaxNG, decltype(&xmlRelaxNGFree)> schema_;
  /// The validation context.
  std::unique_ptr<xmlRelaxNGValidCtxt, decltype(&xmlRelaxNGFreeValidCtxt)>
      valid_ctxt_;
};

}  // namespace scram::xml
