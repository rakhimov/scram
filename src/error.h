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

/// @file error.h
/// Exceptions for SCRAM.

#ifndef SCRAM_SRC_ERROR_H_
#define SCRAM_SRC_ERROR_H_

#include <exception>
#include <string>

#include <boost/current_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/exception.hpp>

/// Convenience macro to throw SCRAM exceptions.
/// This is similar to BOOST_THROW_EXCEPTION;
/// however, it doesn't obfuscate
/// the resultant exception type to conform to boost::exception.
///
/// @param[in] err  The error type deriving from boost::exception.
#define SCRAM_THROW(err)                                                       \
  throw err << ::boost::throw_function(BOOST_THROW_EXCEPTION_CURRENT_FUNCTION) \
            << ::boost::throw_file(__FILE__) << ::boost::throw_line(__LINE__)

namespace scram {

/// The Error class is the base class
/// for common exceptions specific to the SCRAM code.
class Error : virtual public std::exception, virtual public boost::exception {
 public:
  /// Constructs a new error with a provided message.
  ///
  /// @param[in] msg  The message to be passed with this error.
  explicit Error(std::string msg) : msg_(std::move(msg)) {}

  Error(const Error&) = default;  ///< Explicit declaration.

  virtual ~Error() noexcept = default;

  /// @returns The formatted error message to be printed.
  const char* what() const noexcept final { return msg_.c_str(); }

 private:
  std::string msg_;  ///< The error message.
};

/// For input/output related errors.
struct IOError : public Error {
  using Error::Error;
};

/// Dynamic library errors.
struct DLError : public Error {
  using Error::Error;
};

/// Signals internal logic errors,
/// for example, pre-condition failure
/// or use of functionality in ways not designed to.
struct LogicError : public Error {
  using Error::Error;
};

/// This error can be used to indicate
/// that call for a function or operation is not legal.
struct IllegalOperation : public Error {
  using Error::Error;
};

/// The error in analysis settings.
struct SettingsError : public Error {
  using Error::Error;
};

namespace mef {  // MEF specific errors.

/// The MEF container element as namespace.
using errinfo_container = boost::error_info<struct tag_contianer, std::string>;

/// For validating input parameters or user arguments.
struct ValidityError : public Error {
  using Error::Error;
};

/// For cases when events or practically anything is redefined.
struct RedefinitionError : public ValidityError {
  using ValidityError::ValidityError;
};

/// This error indicates that arguments must be unique.
struct DuplicateArgumentError : public ValidityError {
  using ValidityError::ValidityError;
};

/// The error for undefined elements in a model.
struct UndefinedElement : public ValidityError {
  using ValidityError::ValidityError;
};

/// Signals unacceptable cycles in invalid structures.
struct CycleError : public ValidityError {
  using ValidityError::ValidityError;
};

/// Invalid domain for values or arguments.
struct DomainError : public ValidityError {
  using ValidityError::ValidityError;
};

}  // namespace mef

namespace xml {

/// The base for all XML related errors.
struct Error : public scram::Error {
  using scram::Error::Error;
};

/// XML parsing errors.
struct ParseError : public Error {
  using Error::Error;
};

/// XInclude resolution issues.
struct XIncludeError : public Error {
  using Error::Error;
};

/// XML document validity errors.
struct ValidityError : public Error {
  using Error::Error;
};

/// The XML attribute name.
using errinfo_attribute =
    boost::error_info<struct tag_xml_attribute, std::string>;

/// The XML element name.
using errinfo_element = boost::error_info<struct tag_xml_element, std::string>;

}  // namespace xml

}  // namespace scram

#endif  // SCRAM_SRC_ERROR_H_
