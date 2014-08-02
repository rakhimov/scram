/// @file error.cc
/// Implementation of the exceptions for use in SCRAM.
#include "error.h"

namespace scram {

const std::string Error::kPrefix("scram error: ");

Error::Error(std::string msg) : msg_(msg) {}

const char* Error::what() const throw() {
  std::string thrown = Error::kPrefix + msg_;
  return thrown.c_str();
}

}  // namespace scram
