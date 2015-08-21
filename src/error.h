/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

namespace scram {

/// @class Error
/// The Error class is the base class
/// for common exceptions specific to the SCRAM code.
class Error : public std::exception {
 public:
  /// Constructs a new error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit Error(std::string msg);

  /// @returns The error message.
  virtual const char* what() const throw();

  /// @returns The error message.
  const std::string& msg() const { return msg_; }

  /// Sets the error message.
  ///
  /// @param[in] msg The error message.
  void msg(std::string msg) {
    msg_ = msg;
    thrown_ = kPrefix_ + msg;
  }

  virtual ~Error() throw() {}

 protected:
  /// The error message.
  std::string msg_;

 private:
  static const std::string kPrefix_;  ///< Prefix specific to SCRAM.
  std::string thrown_;  ///< The message to throw with the prefix.
};

/// @class ValueError
/// For values that are not acceptable.
/// For example, negative probability.
class ValueError : public Error {
 public:
  /// Constructs a new value error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit ValueError(std::string msg) : Error(msg) {}
};

/// @class ValidationError
/// For validating input parameters or user arguments.
class ValidationError : public Error {
 public:
  /// Constructs a new validation error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit ValidationError(std::string msg) : Error(msg) {}
};

/// @class RedefinitionError
/// For cases when events or practically anything is redefined.
class RedefinitionError : public ValidationError {
 public:
  /// Constructs a new redefinition error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit RedefinitionError(std::string msg) : ValidationError(msg) {}
};

/// @class DuplicateArgumentError
/// This error indicates that arguments must be unique.
class DuplicateArgumentError : public ValidationError {
 public:
  /// Constructs a new duplicate argument error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit DuplicateArgumentError(std::string msg) : ValidationError(msg) {}
};

/// @class IOError
/// For input/output related errors.
class IOError : public Error {
 public:
  /// Constructs a new io error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit IOError(std::string msg) : Error(msg) {}
};

/// @class InvalidArgument
/// This error class can be used
/// to indicate unacceptable arguments.
class InvalidArgument : public Error {
 public:
  /// Constructs a new invalid argument error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit InvalidArgument(std::string msg) : Error(msg) {}
};

/// @class LogicError
/// Signals internal logic errors,
/// for example, pre-condition failure
/// or use of functionality in ways not designed to.
class LogicError : public Error {
 public:
  /// Constructs a new logic error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit LogicError(std::string msg) : Error(msg) {}
};

/// @class IllegalOperation
/// This error can be used to indicate
/// that call for a function or operation is not legal.
/// For example, a derived class can make illegal
/// the call of the virtual function of the base class.
class IllegalOperation : public Error {
 public:
  /// Constructs a new illegal operation error with a provided message.
  ///
  /// @param[in] msg The message to be passed with this error.
  explicit IllegalOperation(std::string msg) : Error(msg) {}
};

}  // namespace scram

#endif  // SCRAM_SRC_ERROR_H_
