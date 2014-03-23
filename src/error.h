// exceptions for scram project
#ifndef SCRAM_ERROR_H_
#define SCRAM_ERROR_H_

#include <exception>
#include <string>

namespace scram {

class Error : public std::exception {
 public:
  // Constructs for a new error with a default message
  Error();

  // Constructs for a new error with a provided message
  Error(std::string msg);

  // Returns the error message
  virtual const char* what() const throw();

  // Returns the error message
  std::string msg() const {
    return msg_;
  }

  // Sets the error message
  void msg(std::string msg) {
    msg_ = msg;
  }

  virtual ~Error() throw() {}

 protected:
  // The error message
  std::string msg_;

 private:
  static const std::string kPrefix;
};

// For values that are not acceptable
// For example, negative probability
class ValueError : public Error {
 public:
  ValueError(std::string msg) : Error(msg) {}
};

// For validating input parameters
class ValidationError : public Error {
 public:
  ValidationError(std::string msg) : Error(msg) {}
};

}  // namespace scram

#endif  // SCRAM_ERROR_H_
