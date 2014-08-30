/// @file error.cc
/// Implementation of the exceptions for use in SCRAM.
#include "error.h"

namespace scram {

const std::string Error::kPrefix_("scram error: ");

Error::Error(std::string msg) : msg_(msg), thrown_(Error::kPrefix_ + msg) {}

const char* Error::what() const throw() {
  return thrown_.c_str();
}

}  // namespace scram
