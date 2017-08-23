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

#ifndef SCRAM_SRC_XML_H_
#define SCRAM_SRC_XML_H_

#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/utility/string_ref.hpp>

#include <libxml/parser.h>
#include <libxml/relaxng.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>

#include "error.h"

namespace scram {

namespace xml {

class Element;

}  // namespace xml

std::string GetLine(const xml::Element& xml_node);  // For error reporting.

namespace xml {

using string_view = boost::string_ref;  ///< Non-owning, immutable string view.

namespace detail {  // Internal XML helper functions.

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
CastValue(const xml::string_view& value);

/// Specialization for integer values.
template <>
inline int CastValue<int>(const xml::string_view& value) {
  char* end_char = nullptr;
  std::int64_t ret = std::strtoll(value.data(), &end_char, 10);
  int len = end_char - value.data();
  if (len != value.size() || ret > std::numeric_limits<int>::max() ||
      ret < std::numeric_limits<int>::min())
    throw ValidationError("Failed to interpret '" + value.to_string() +
                          "' to 'int'.");
  return ret;
}

/// Specialization for floating point numbers.
template <>
inline double CastValue<double>(const xml::string_view& value) {
  char* end_char = nullptr;
  double ret = std::strtod(value.data(), &end_char);
  int len = end_char - value.data();
  if (len != value.size() || ret == HUGE_VAL || ret == -HUGE_VAL)
    throw ValidationError("Failed to interpret '" + value.to_string() +
                          "' to 'double'.");
  return ret;
}

/// Specialization for Boolean values.
template <>
inline bool CastValue<bool>(const xml::string_view& value) {
  if (value == "true" || value == "1")
    return true;
  if (value == "false" || value == "0")
    return false;
  throw ValidationError("Failed to interpret '" + value.to_string() +
                        "' to 'bool'.");
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
inline xml::string_view trim(const xml::string_view& text) noexcept {
  auto pos_first = text.find_first_not_of(" ");
  if (pos_first == xml::string_view::npos)
    return {};

  auto pos_last = text.find_last_not_of(" ");
  auto len = pos_last - pos_first + 1;

  return xml::string_view(text.data() + pos_first, len);
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

    /// Extracts the element by its position.
    /// This is a temporary helper function to move from xmlpp::NodeSet.
    /// Use iterators and loops instead.
    ///
    /// @param[in] pos  The position of the element.
    ///
    /// @returns The element on the position.
    ///
    /// @throws std::out_of_range  The position is invalid.
    ///
    /// @note O(N) complexity unlike xmlpp::NodeSet O(1).
    ///
    /// @todo Remove.
    Element at(std::size_t pos) const {
      auto it = std::next(begin(), pos);
      if (it == end())
        throw std::out_of_range("The position is out of range.");
      return *it;
    }

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
  xml::string_view filename() const {
    return detail::from_utf8(element_->doc->URL);
  }

  /// @returns The line number of the element.
  int line() const { return XML_GET_LINE(to_node()); }

  /// @returns The name of the XML element.
  ///
  /// @pre The element has a name.
  xml::string_view name() const { return detail::from_utf8(element_->name); }

  /// Retrieves the XML element's attribute values.
  ///
  /// @param[in] name  The name of the requested attribute.
  ///
  /// @returns The attribute value or
  ///          empty string if no attribute (optional attribute).
  ///
  /// @pre XML attributes never contain empty strings.
  /// @pre XML attribute values are simple texts w/o DTD processing.
  xml::string_view attribute(const char* name) const {
    const xmlAttr* property = xmlHasProp(to_node(), detail::to_utf8(name));
    if (!property)
      return {};
    const xmlNode* text_node = property->children;
    assert(text_node && text_node->type == XML_TEXT_NODE);
    assert(text_node->content);
    return detail::trim(detail::from_utf8(text_node->content));
  }

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

  /// @returns The XML element's text.
  ///
  /// @pre The Element has text.
  xml::string_view text() const {
    const xmlNode* text_node = element_->children;
    while (text_node && text_node->type != XML_TEXT_NODE)
      text_node = text_node->next;
    assert(text_node && "Element does not have text.");
    assert(text_node->content && "Missing text in Element.");
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
  /// @throws ValidationError  Casting is unsuccessful.
  template <typename T>
  std::enable_if_t<std::is_arithmetic<T>::value, boost::optional<T>>
  attribute(const char* name) const {
    xml::string_view value = attribute(name);
    if (value.empty())
      return {};
    try {
      return detail::CastValue<T>(value);
    } catch (ValidationError& err) {
      err.msg(GetLine(*this) + "Attribute '" + name + "': " + err.msg());
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
      return detail::CastValue<T>(text());
    } catch (ValidationError& err) {
      err.msg(GetLine(*this) + "Text element: " + err.msg());
      throw;
    }
  }

  /// @param[in] name  The name of the child element.
  ///                  Empty string to request any first child element.
  ///
  /// @returns The first child element (with the given name).
  boost::optional<Element> child(xml::string_view name = "") const {
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
  auto children(xml::string_view name) const {
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

/// XML DOM tree document.
class Document {
 public:
  /// @param[in] doc  Fully parsed document.
  explicit Document(const xmlDoc* doc) : doc_(doc) { assert(doc_); }

  /// Move constructor to transfer the document ownership.
  Document(Document&& other) noexcept : doc_(other.doc_) {
    other.doc_ = nullptr;
  }

  /// Frees the underlying document.
  ~Document() {
    if (doc_)
      xmlFreeDoc(const_cast<xmlDoc*>(doc_));
  }

  /// @returns The root element of the document.
  ///
  /// @pre The document has a root node.
  Element root() const {
    return Element(reinterpret_cast<const xmlElement*>(
        xmlDocGetRootElement(const_cast<xmlDoc*>(doc_))));
  }

  /// @returns The underlying data document.
  const xmlDoc* get() const { return doc_; }

 private:
  const xmlDoc* doc_;  ///< The XML DOM document.
};

/// RelaxNG validator.
class Validator {
 public:
  /// @param[in] rng_file  The path to the schema file.
  ///
  /// @throws The library provided error for invalid XML RNG schema file.
  ///
  /// @todo Properly wrap the exception for invalid schema files.
  explicit Validator(const std::string& rng_file) {
    struct ParserCtxtDeleter {
      void operator()(xmlRelaxNGParserCtxt* ctxt) noexcept {
        if (ctxt)
          xmlRelaxNGFreeParserCtxt(ctxt);
      }
    };
    std::unique_ptr<xmlRelaxNGParserCtxt, ParserCtxtDeleter> parser_ctxt(
        xmlRelaxNGNewParserCtxt(rng_file.c_str()));
    assert(parser_ctxt);  ///< @todo Provide rng parser errors.
    schema_.reset(xmlRelaxNGParse(parser_ctxt.get()));
    assert(schema_);  ///< @todo Provide schema parsing errors.
    valid_ctxt_.reset(xmlRelaxNGNewValidCtxt(schema_.get()));
    assert(valid_ctxt_);  ///< @todo Provide valid ctxt initialization error.
  }

  /// Validates XML DOM documents against the schema.
  ///
  /// @param[in] doc  The initialized XML DOM document.
  ///
  /// @throws ValidationError  The document failed schema validation.
  void validate(const Document& doc) {
    int ret = xmlRelaxNGValidateDoc(valid_ctxt_.get(),
                                    const_cast<xmlDoc*>(doc.get()));
    /// @todo Provide validation error messages.
    if (ret > 0)
      throw ValidationError("Document failed schema validation:\n");
    assert(ret == 0);  ///< Handle XML internal errors.
  }

 private:
  /// Deleter of the schema.
  struct SchemaDeleter {
    /// Frees schema with the library call.
    void operator()(xmlRelaxNG* schema) noexcept {
      if (schema)
        xmlRelaxNGFree(schema);
    }
  };
  /// Deleter of the validation context.
  struct ValidCtxtDeleter {
    /// Frees validation context with the library call.
    void operator()(xmlRelaxNGValidCtxt* ctxt) noexcept {
      if (ctxt)
        xmlRelaxNGFreeValidCtxt(ctxt);
    }
  };
  /// The schema used by the validation context.
  std::unique_ptr<xmlRelaxNG, SchemaDeleter> schema_;
  /// The validation context.
  std::unique_ptr<xmlRelaxNGValidCtxt, ValidCtxtDeleter> valid_ctxt_;
};

/// The parser options passed to the library parser.
const int kParserOptions = XML_PARSE_XINCLUDE | XML_PARSE_NOBASEFIX |
                           XML_PARSE_NONET | XML_PARSE_NOXINCNODE |
                           XML_PARSE_COMPACT | XML_PARSE_HUGE;

/// Parses XML input document.
/// All XInclude directives are processed into the final document.
///
/// @param[in] file_path  The path to the document file.
/// @param[in] validator  Optional validator against the RNG schema.
///
/// @returns The initialized document.
///
/// @throws ValidationError  There are problems loading the XML file.
///
/// @todo Provide proper validation error messages.
inline Document Parse(const std::string& file_path,
                      Validator* validator = nullptr) {
  xmlDoc* doc = xmlReadFile(file_path.c_str(), nullptr, kParserOptions);
  if (!doc)
      throw ValidationError("XML file is invalid:\n");
  if (xmlXIncludeProcessFlags(doc, kParserOptions) < 0)
    throw ValidationError("XML Xinclude substitutions are failed.");
  Document manager(doc);
  if (validator)
    validator->validate(manager);
  return manager;
}

/// Convenience overload to parse the XML in memory.
inline Document ParseMemory(const std::string& raw,
                            Validator* validator = nullptr) {
  xmlDoc* doc =
      xmlReadMemory(raw.c_str(), raw.size() + 1, "", nullptr, kParserOptions);
  if (!doc)
    throw ValidationError("XML in memory is invalid.");
  if (xmlXIncludeProcessFlags(doc, kParserOptions) < 0)
    throw ValidationError("XML Xinclude substitutions are failed.");
  Document manager(doc);
  if (validator)
    validator->validate(manager);
  return manager;
}

}  // namespace xml

/// Returns XML line number message.
inline std::string GetLine(const xml::Element& xml_node) {
  return "Line " + std::to_string(xml_node.line()) + ":\n";
}

}  // namespace scram

#endif  // SCRAM_SRC_XML_H_
