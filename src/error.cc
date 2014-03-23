#include "error.h"

const std::string Error::kPrefix("scram error: ");

Error::Error(std::string msg) : msg_(msg) {}

const char* Error::what() const throw() {
  std::string thrown = Error::kPrefix + msg_;
  return thrown.c_str();
}
