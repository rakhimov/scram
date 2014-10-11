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

}  // namespace scram

#endif  // SCRAM_SRC_ERROR_H_
