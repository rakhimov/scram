/// @file error.h
/// Exceptions for scram project.
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
  /// @param[in] msg The message to be passed with this error.
  explicit Error(std::string msg);

  /// @returns The error message.
  virtual const char* what() const throw();

  /// @returns The error message.
  const std::string& msg() const { return msg_; }

  /// Sets the error message.
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
  /// SCRAM prefix specific to the application.
  static const std::string kPrefix_;

  /// The message to throw with the prefix.
  std::string thrown_;
};

/// @class ValueError
/// For values that are not acceptable.
/// For example, negative probability.
class ValueError : public Error {
 public:
  /// Constructs a new value error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit ValueError(std::string msg) : Error(msg) {}
};

/// @class ValidationError
/// For validating input parameters or user arguments.
class ValidationError : public Error {
 public:
  /// Constructs a new validation error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit ValidationError(std::string msg) : Error(msg) {}
};

/// @class IOError
/// For input/output related errors.
class IOError : public Error {
 public:
  /// Constructs a new io error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit IOError(std::string msg) : Error(msg) {}
};

/// @class DoubleDefinition
/// For cases when events or practically anything is defined several times.
class DoubleDefinition : public Error {
 public:
  /// Constructs a new double definition error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit DoubleDefinition(std::string msg) : Error(msg) {}
};

/// @class InvalidArgument
/// This error class can be used to indicate unacceptable arguments.
class InvalidArgument : public Error {
 public:
  /// Constructs a new invalid argument error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit InvalidArgument(std::string msg) : Error(msg) {}
};

/// @class LogicError
/// Signals internal logic errors, for example, pre-condition failure
/// or use of functionality in ways not designed to.
class LogicError : public Error {
 public:
  /// Constructs a new logic error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit LogicError(std::string msg) : Error(msg) {}
};

/// @class IllegalOperation
/// This error can be used to indicate that call for a method is not
/// what is intended from an object derived from another class that actually
/// supports the operation. This is a strong kind of logic error.
class IllegalOperation : public Error {
 public:
  /// Constructs a new illegal operation error with a provided message.
  /// @param[in] msg The message to be passed with this error.
  explicit IllegalOperation(std::string msg) : Error(msg) {}
};

}  // namespace scram

#endif  // SCRAM_SRC_ERROR_H_
