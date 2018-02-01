/*
 * Copyright (C) 2016-2018 Olzhas Rakhimov
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
/// Facilities to stream data in XML format.

#pragma once

#include <cassert>
#include <cstdio>

#include <algorithm>
#include <exception>
#include <string>

#include <boost/exception/errinfo_errno.hpp>

#include "error.h"

namespace scram::xml {

/// Errors in using XML streaming facilities.
struct StreamError : public Error {
  using Error::Error;
};

namespace detail {  // XML streaming helpers.

const char kIndentChar = ' ';  ///< The whitespace character.
const int kMaxIndent = 20;  ///< The maximum number of characters.

/// Manages XML element indentation upon output.
///
/// @pre Indenter instance is not shared between XML Streams.
///
/// @note The indentation is bounded by kMaxIndent.
///       If indentation exceeds kMaxIndent,
///       only kMaxIndent indentation will be produced w/o error.
class Indenter {
 public:
  /// RAII class to manage the current indentation string.
  class Indentation {
   public:
    /// @param[in] num_chars  The number of indentation characters.
    /// @param[in,out] indent  The provider of the indentation buffer.
    ///
    /// @pre There's only a single instance at a time.
    /// @pre The indent buffer is filled w/ kIndentChar up to kMaxIndent.
    ///
    /// @post If the number of chars exceeds kMaxIndent,
    ///       the indentation is kMaxIndent.
    Indentation(int num_chars, Indenter* indent)
        : pos_(num_chars > kMaxIndent ? kMaxIndent : num_chars),
          indent_(indent) {
      indent_->spaces[pos_] = '\0';  // Indent string up to the position.
    }

    /// Restores the buffer.
    ~Indentation() { indent_->spaces[pos_] = kIndentChar; }

    /// @returns The indentation string.
    operator const char*() { return indent_->spaces; }

   private:
    int pos_;  ///< The position of the null char in the string.
    Indenter* indent_;  ///< The string buffer provider.
  };

  /// Initializes the buffer with enough space characters.
  ///
  /// @param[in] indent  Enable/disable indentation.
  explicit Indenter(bool indent = true) : indent_(indent) {
    std::fill_n(spaces, kMaxIndent, kIndentChar);
  }

  /// @param[in] num_chars  The number of indentation characters.
  ///
  /// @returns Indentation to produce the representative string.
  Indentation operator()(int num_chars) {
    return indent_ ? Indentation(num_chars, this) : Indentation(0, this);
  }

 private:
  bool indent_;  ///< Option to enable/disable indentation.
  char spaces[kMaxIndent + 1];  ///< The indentation and terminator.
};

/// Adaptor for stdio FILE stream with write generic interface.
///
/// @note Write operations do not return any error code or throw exceptions.
///       If any IO errors happen,
///       the FILE handler contains the error information.
class FileStream {
 public:
  /// @param[in] file  The output file stream.
  explicit FileStream(std::FILE* file) : file_(file) {}

  /// @returns The destination file stream.
  std::FILE* file() { return file_; }

  /// Writes a value into file.
  /// @{
  void write(const std::string& value) { write(value.c_str()); }
  void write(const char* value) { std::fputs(value, file_); }
  void write(const char value) { std::fputc(value, file_); }
  void write(int value) {
    if (value < 0) {
      std::fputc('-', file_);
      value = -value;
    }
    write(static_cast<std::size_t>(value));
  }
  void write(std::size_t value) {
    char temp[20];
    char* p = temp;
    do {
      *p++ = value % 10 + '0';
      value /= 10;
    } while (value > 0);

    do {
      std::fputc(*--p, file_);
    } while (p != temp);
  }
  void write(double value) { std::fprintf(file_, "%g", value); }
  /// @}

 private:
  std::FILE* file_;  ///< The destination file.
};

/// Convenience wrapper to provide C++ stream-like interface.
template <typename T>
FileStream& operator<<(FileStream& file, T&& value) {
  file.write(std::forward<T>(value));
  return file;
}

}  // namespace detail

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
/// @pre All strings are UTF-8 encoded.
///
/// @note The stream is not designed to mix XML text and elements
///       due to the absence of the use case or need.
///
/// @warning The names of elements and attributes
///          are NOT validated to be proper XML NCName.
///          It is up to the caller
///          to sanitize the names (<, >, &, ", ', :, etc.).
///
/// @warning The API works with C strings,
///          but this class does not manage the string lifetime.
///          It doesn't own any strings.
///          The provider of the strings must make sure
///          the lifetime of the string is long enough for streaming.
///          It is the most common case that strings are literals,
///          so there's no need to worry about dynamic lifetime.
class StreamElement {
 public:
  /// Constructs a root streamer for the XML element data
  /// ready to accept attributes, elements, text.
  ///
  /// @param[in] name  Non-empty string name for the element.
  /// @param[in] indenter  The indentation provider.
  /// @param[in,out] out  The destination stream.
  ///
  /// @throws StreamError  Invalid setup for the element.
  StreamElement(const char* name, detail::Indenter* indenter,
                detail::FileStream* out)
      : StreamElement(name, 0, nullptr, indenter, out) {}

  /// Puts the closing tag.
  ///
  /// @pre No child element is alive.
  ///
  /// @warning The output will be malformed
  ///          if the child outlives the parent.
  ///          It can happen if the destructor is called explicitly,
  ///          or if the objects are allocated on the heap
  ///          with different lifetimes.
  ~StreamElement() noexcept {
    assert(active_ && "The child element may still be alive.");
    assert(!(parent_ && parent_->active_) && "The parent must be inactive.");
    if (parent_)
      parent_->active_ = true;
    if (accept_attributes_) {
      out_ << "/>\n";
    } else if (accept_elements_) {
      out_ << indenter_(kIndent_);
    closing_tag:
      out_ << "</" << kName_ << ">\n";
    } else {
      assert(accept_text_ && "The element is in unspecified state.");
      goto closing_tag;
    }
  }

  /// Sets the attributes for the element.
  ///
  /// @tparam T  Streamable type supporting operator<<.
  ///
  /// @param[in] name  Non-empty name for the attribute.
  /// @param[in] value  The value of the attribute.
  ///
  /// @returns The reference to this element.
  ///
  /// @post &, <, " chars are escaped in the value text.
  ///
  /// @throws StreamError  Invalid setup for the attribute.
  template <typename T>
  StreamElement& SetAttribute(const char* name, T&& value) {
    if (!active_)
      throw StreamError("The element is inactive.");
    if (!accept_attributes_)
      throw StreamError("Too late for attributes.");
    if (*name == '\0')
      throw StreamError("Attribute name can't be empty.");

    out_ << " " << name << "=\"";
    PutValue(std::forward<T>(value));
    out_ << "\"";
    return *this;
  }

  /// Adds text to the element.
  ///
  /// @tparam T  Streamable type supporting operator<<.
  ///
  /// @param[in] text  Non-empty text.
  ///
  /// @returns The reference to this element.
  ///
  /// @post No more elements or attributes can be added.
  /// @post More text can be added.
  /// @post &, <, " chars are escaped in the text.
  ///
  /// @throws StreamError  Invalid setup or state for text addition.
  template <typename T>
  StreamElement& AddText(T&& text) {
    if (!active_)
      throw StreamError("The element is inactive.");
    if (!accept_text_)
      throw StreamError("Too late to put text.");

    if (accept_elements_)
      accept_elements_ = false;
    if (accept_attributes_) {
      accept_attributes_ = false;
      out_ << ">";
    }
    PutValue(std::forward<T>(text));
    return *this;
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
  /// @throws StreamError  Invalid setup or state for element addition.
  StreamElement AddChild(const char* name) {
    if (!active_)
      throw StreamError("The element is inactive.");
    if (!accept_elements_)
      throw StreamError("Too late to add elements.");
    if (*name == '\0')
      throw StreamError("Element name can't be empty.");

    if (accept_text_)
      accept_text_ = false;
    if (accept_attributes_) {
      accept_attributes_ = false;
      out_ << ">\n";
    }
    return StreamElement(name, kIndent_ + kIndentIncrement, this, &indenter_,
                         &out_);
  }

 private:
  static const int kIndentIncrement = 2;  ///< The number of chars per indent.

  /// Private constructor for a streamer
  /// to pass parent-child information.
  ///
  /// @param[in] name  Non-empty string name for the element.
  /// @param[in] indent  The number of spaces to indent the tags.
  /// @param[in,out] parent  The parent element.
  ///                        Null pointer for root streamers.
  /// @param[in] indenter  The indentation provider.
  /// @param[in,out] out  The destination stream.
  ///
  /// @throws StreamError  Invalid setup for the element.
  StreamElement(const char* name, int indent, StreamElement* parent,
                detail::Indenter* indenter, detail::FileStream* out)
      : kName_(name),
        kIndent_(indent),
        accept_attributes_(true),
        accept_elements_(true),
        accept_text_(true),
        active_(true),
        parent_(parent),
        indenter_(*indenter),
        out_(*out) {
    if (*kName_ == '\0')
      throw StreamError("The element name can't be empty.");

    if (parent_) {
      if (!parent_->active_)
        throw StreamError("The parent is inactive.");
      parent_->active_ = false;
    }
    assert(kIndent_ >= 0 && "Negative XML indentation.");
    out_ << indenter_(kIndent_) << "<" << kName_;
  }

  /// Puts the value as text escaping the required XML special characters.
  /// @{
  void PutValue(int value) { out_ << value; }
  void PutValue(double value) { out_ << value; }
  void PutValue(std::size_t value) { out_ << value; }
  void PutValue(bool value) { out_ << (value ? "true" : "false"); }
  void PutValue(const std::string& value) { PutValue(value.c_str()); }
  void PutValue(const char* value) {
    bool has_special_chars = [value]() mutable {  // 2x faster than strpbrk.
      for (;; ++value) {
        switch (*value) {
          case '\0':
            return false;
          case '&':
          case '<':
          case '"':
            return true;
        }
      }
    }();

    if (!has_special_chars) {  // The most common case.
      out_ << value;
    } else {
      for (;; ++value) {
        switch (*value) {
          case '\0':
            return;
          case '&':
            out_ << "&amp;";
            break;
          case '<':
            out_ << "&lt;";
            break;
          case '"':
            out_ << "&quot;";
            break;
          default:
            out_ << *value;
        }
      }
    }
  }
  /// @}

  const char* kName_;  ///< The name of the element.
  const int kIndent_;  ///< Indentation for tags.
  bool accept_attributes_;  ///< Flag for preventing late attributes.
  bool accept_elements_;  ///< Flag for preventing late elements.
  bool accept_text_;  ///< Flag for preventing late text additions.
  bool active_;  ///< Active in streaming.
  StreamElement* parent_;  ///< Parent element.
  detail::Indenter& indenter_;  ///< The indentation string producer.
  detail::FileStream& out_;  ///< The output destination.
};

/// XML Stream document.
///
/// @pre Only this stream and its elements write to the output destination.
///      No other writes happen while this stream is alive.
///
/// @note The document elements are indented up to 10 levels for readability.
///       The XML tree depth beyond 10 elements is printed at level 10.
class Stream {
 public:
  /// Constructs a document with XML header.
  ///
  /// @param[in] out  The stream destination.
  /// @param[in] indent  Option to indent output for readability.
  ///
  /// @note This output file has clean error state.
  explicit Stream(std::FILE* out, bool indent = true)
      : indenter_(indent),
        has_root_(false),
        uncaught_exceptions_(std::uncaught_exceptions()),
        out_(out) {
    assert(!std::ferror(out) && "Unclean error state in output destination.");
    out_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  }

  /// @throws IOError  The file write operation has failed.
  ///
  /// @post The exception is thrown only if no other exception is on flight.
  ~Stream() noexcept(false) {
    int err = std::ferror(out_.file());
    if (err && (std::uncaught_exceptions() == uncaught_exceptions_))
      SCRAM_THROW(IOError("FILE error on write")) << boost::errinfo_errno(err);
  }

  /// Creates a root element for the document.
  ///
  /// @param[in] name  The name for the root element.
  ///
  /// @returns XML stream element representing the document root.
  ///
  /// @pre The document is alive at least as long as the created root.
  ///
  /// @throws StreamError  The document already has a root element,
  ///                      or root element construction has failed.
  StreamElement root(const char* name) {
    if (has_root_)
      throw StreamError("The XML stream document already has a root.");
    StreamElement element(name, &indenter_, &out_);
    has_root_ = true;
    return element;
  }

 private:
  detail::Indenter indenter_;  ///< The indentation manager for the document.
  bool has_root_;  ///< The document has constructed its root.
  int uncaught_exceptions_;  ///< The balance of exceptions.
  detail::FileStream out_;  ///< The output stream.
};

}  // namespace scram::xml
