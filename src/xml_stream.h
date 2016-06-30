/*
 * Copyright (C) 2016 Olzhas Rakhimov
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

/// @file xml_stream.h
/// Facilities to stream data in XML format.

#ifndef SCRAM_SRC_XML_STREAM_H_
#define SCRAM_SRC_XML_STREAM_H_

#include <ostream>

#include "error.h"

namespace scram {

/// Errors in using XML streaming facilities.
struct XmlStreamError : public Error {
  using Error::Error;  ///< Constructor with a message.
};

/// Writer of data formed as an XML element to a stream.
/// This class relies on the RAII to put the closing tags.
/// It is designed for stack-based use
/// so that its destructor gets called at scope exit.
/// One element at a time must be operated.
/// The output will be malformed
/// if two child elements at the same scope are being streamed.
/// To prevent this from happening,
/// the parent element is put into an inactive state
/// while its child element is alive.
///
/// @warning The names of elements and contents of XML data
///          are NOT fully validated to be proper XML.
///          It is up to the caller
///          to sanitize the input text.
///
/// @warning The API works with C strings,
///          but this class does not manage the string lifetime.
///          It doesn't own any strings.
///          The provider of the strings must make sure
///          the lifetime of the string is long enough for streaming.
///          It is the most common case that strings are literals,
///          so there's no need to worry about dynamic lifetime.
class XmlStreamElement {
 public:
  /// Constructs a root streamer for the XML element data
  /// ready to accept attributes, elements, text.
  ///
  /// @param[in] name  Non-empty string name for the element.
  /// @param[in,out] out  The destination stream.
  ///
  /// @throws XmlStreamError  Invalid setup for the element.
  XmlStreamElement(const char* name, std::ostream& out);

  XmlStreamElement& operator=(const XmlStreamElement&) = delete;

  /// Copy constructor is only declared
  /// to make the compiler happy.
  /// The code must rely on the RVO, NRVO, and copy elision
  /// instead of this constructor.
  ///
  /// The constructor is not defined,
  /// so the use of this constructor will produce a linker error.
  XmlStreamElement(const XmlStreamElement&);

  /// Puts the closing tag.
  ///
  /// @pre No child element is alive.
  ///
  /// @warning The output will be malformed
  ///          if the child outlives the parent.
  ///          It can happen if the destructor is called explicitly,
  ///          or if the objects are allocated on the heap
  ///          with different lifetimes.
  ~XmlStreamElement() noexcept;

  /// Sets the attributes for the element.
  ///
  /// @tparam T  Streamable type supporting operator<<.
  ///
  /// @param[in] name  Non-empty name for the attribute.
  /// @param[in] value  The value of the attribute.
  ///
  /// @returns The reference to this element.
  ///
  /// @throws XmlStreamError  Invalid setup for the attribute.
  template <typename T>
  XmlStreamElement& SetAttribute(const char* name, T&& value) {
    if (!active_) throw XmlStreamError("The element is inactive.");
    if (!accept_attributes_) throw XmlStreamError("Too late for attributes.");
    if (*name == '\0') throw XmlStreamError("Attribute name can't be empty.");
    out_ << " " << name << "=\"" << std::forward<T>(value) << "\"";
    return *this;
  }

  /// Adds text to the element.
  ///
  /// @tparam T  Streamable type supporting operator<<.
  ///
  /// @param[in] text  Non-empty text.
  ///
  /// @post No more elements or attributes can be added.
  /// @post More text can be added.
  ///
  /// @throws XmlStreamError  Invalid setup or state for text addition.
  template <typename T>
  void AddText(T&& text) {
    if (!active_) throw XmlStreamError("The element is inactive.");
    if (!accept_text_) throw XmlStreamError("Too late to put text.");
    if (accept_elements_) accept_elements_ = false;
    if (accept_attributes_) {
      accept_attributes_ = false;
      out_ << ">";
    }
    out_ << std::forward<T>(text);
  }

  /// Adds a child element to the element.
  ///
  /// @param[in] name  Non-empty name for the child element.
  ///
  /// @returns A streamer for child element.
  ///
  /// @pre The child element will be destroyed before the parent.
  ///
  /// @post The parent element is inactive
  ///       while the child element is alive.
  ///
  /// @throws XmlStreamError  Invalid setup or state for element addition.
  XmlStreamElement AddChild(const char* name);

 private:
  /// Private constructor for a streamer
  /// to pass parent-child information.
  ///
  /// @param[in] name  Non-empty string name for the element.
  /// @param[in] indent  The number of spaces to indent the tags.
  /// @param[in,out] parent  The parent element.
  ///                        Null pointer for root streamers.
  /// @param[in,out] out  The destination stream.
  ///
  /// @throws XmlStreamError  Invalid setup for the element.
  XmlStreamElement(const char* name, int indent, XmlStreamElement* parent,
                   std::ostream& out);

  const char* kName_;  ///< The name of the element.
  const int kIndent_;  ///< Indentation for tags.
  bool accept_attributes_;  ///< Flag for preventing late attributes.
  bool accept_elements_;  ///< Flag for preventing late elements.
  bool accept_text_;  ///< Flag for preventing late text additions.
  bool active_;  ///< Active in streaming.
  XmlStreamElement* parent_;  ///< Parent element.
  std::ostream& out_;  ///< The output destination.
};

}  // namespace scram

#endif  // SCRAM_SRC_XML_STREAM_H_
