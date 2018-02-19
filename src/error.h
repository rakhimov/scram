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
/// Exceptions for SCRAM.
/// Exceptions are designed with boost::exception;
/// that is, exception classes act like tags.
/// No new data members shall ever be added to derived exception classes
/// (no slicing upon copy or change of exception type!).
/// Instead, all the data are carried with boost::error_info mechanism.

#pragma once

#include <exception>
#include <string>

#include <boost/current_function.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/info_tuple.hpp>

#include "ext/source_info.h"

/// Convenience macro to throw SCRAM exceptions.
/// This is similar to BOOST_THROW_EXCEPTION;
/// however, it doesn't obfuscate
/// the resultant exception type to conform to boost::exception.
///
/// @param[in] err  The error type deriving from boost::exception.
#define SCRAM_THROW(err)                                                       \
  throw err << ::boost::throw_function(BOOST_THROW_EXCEPTION_CURRENT_FUNCTION) \
            << ::boost::throw_file(FILE_REL_PATH)                              \
            << ::boost::throw_line(__LINE__)

namespace scram {

/// The generic tag to carry an erroneous value.
/// Use this tag only if another more-specific error tag is not available.
using errinfo_value = boost::error_info<struct tag_value, std::string>;

/// The Error class is the base class
/// for all exceptions specific to the SCRAM code.
///
/// @note The copy constructor is not noexcept as required by std::exception.
///       However, this class may only throw std::bad_alloc upon copy,
///       which may be produced anyway
///       even if the copy constructor were noexcept.
class Error : virtual public std::exception, virtual public boost::exception {
 public:
  /// Constructs a new error with a provided message.
  ///
  /// @param[in] msg  The message to be passed with this error.
  explicit Error(std::string msg) : msg_(std::move(msg)) {}

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
/// @{
using errinfo_container_id =
    boost::error_info<struct tag_contianer_id, std::string>;
using errinfo_container_type =
    boost::error_info<struct tag_container_type, const char*>;
using errinfo_container =
    boost::tuple<errinfo_container_id, errinfo_container_type>;
/// @}

/// MEF Element Attribute.
using errinfo_attribute = boost::error_info<struct tag_attribute, std::string>;

/// The MEF element identifier data in errors.
/// @{
using errinfo_element_id =
    boost::error_info<struct tag_element_id, std::string>;
using errinfo_element_type =
    boost::error_info<struct tag_element_type, const char*>;
using errinfo_element = boost::tuple<errinfo_element_id, errinfo_element_type>;
/// @}

/// The MEF element reference.
using errinfo_reference = boost::error_info<struct tag_reference, std::string>;
/// The base path to follow the reference.
using errinfo_base_path = boost::error_info<struct tag_base_path, std::string>;

/// String representation of invalid cycles/loops.
using errinfo_cycle = boost::error_info<struct tag_cycle, std::string>;

/// Connectives in formulae.
using errinfo_connective =
    boost::error_info<struct tag_connective, std::string>;

/// Model validity errors.
struct ValidityError : public Error {
  using Error::Error;
};

/// This error indicates that elements must be unique.
struct DuplicateElementError : public ValidityError {
  DuplicateElementError() : ValidityError("Duplicate Element Error") {}
};

/// The error for undefined elements in a model.
struct UndefinedElement : public ValidityError {
  UndefinedElement() : ValidityError("Undefined Element Error") {}
};

/// Unacceptable cycles in model structures.
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
